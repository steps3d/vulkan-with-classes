//
// Using textures with VulkanWindow
//

#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Texture.h"
#include	"Mesh.h"
#include	"Framebuffer.h"
#include	"Semaphore.h"
#include	"ScreenQuad.h"
#include	"CameraController.h"

struct Ubo
{
	glm::mat4 mv;
	glm::mat4 proj;
	glm::mat4 nm;
	glm::vec4 light;
};

class	DeferredWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	Sampler							sampler;
	Framebuffer						fb;					// G-buffer 
	CommandBuffer					offscreenCmd;// = VK_NULL_HANDLE;
	DescriptorSet					offscreenDescriptorSet;
	Semaphore						offscreenSemaphore;
	GraphicsPipeline				offscreenPipeline;
	ScreenQuad						screen;				// class used to do screen processing
	float							zMin = 0.1f;
	float							zMax = 100.0f;	
	double							time = 0;

	DescriptorSet					offscreenDescriptorSet1, offscreenDescriptorSet2, offscreenDescriptorSet3;

	Mesh * box1 = nullptr;		// decalMap, bump1 -> DS1
	Mesh * box2 = nullptr;		// stoneMap. bump2 -> DS2
	Mesh * box3 = nullptr;
	Mesh * box4 = nullptr;
	Mesh * box5 = nullptr;
	Mesh * knot = nullptr;		// knotMap, bump2 -> DS3

	Texture							decalMap, stoneMap, knotMap;
	Texture							bump1, bump2;

public:
	DeferredWindow ( int w, int h, const std::string& t ) : VulkanWindow ( w, h, t, true )
	{
		sampler.setMinFilter ( VK_FILTER_LINEAR ).setMagFilter ( VK_FILTER_LINEAR ).create ( device );
		screen.create  ( device );

		decalMap.load ( device, "../../Textures/oak.jpg"         );
		stoneMap.load ( device, "../../Textures/block.jpg"       );
		knotMap.load  ( device, "../../Textures/Oxidated.jpg"    );
		bump1.load    ( device, "../../Textures/wood_normal.png" );
		bump2.load    ( device, "../../Textures/brick_nm.bmp"    );

			// create G-buffer with 2 RGBA16F color attachments and depth attachment
		fb.init ( device, getWidth (), getHeight () )
		  .addAttachment ( VK_FORMAT_R16G16B16A16_SNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
		  .addAttachment ( VK_FORMAT_R16G16B16A16_SNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
		  .addAttachment ( VK_FORMAT_D24_UNORM_S8_UINT,  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT  )
		  .create ();
		  
			// create meshes
		createGeometry ();
				
			// create all pipelines
		createPipelines ();
		setController   ( new CameraController ( this ) );
	}

	~DeferredWindow ()
	{
		delete box1;
		delete box2;
		delete box3;
		delete box4;
		delete box5;
		delete knot;
	}

	void	createUniformBuffers ()
	{
		uniformBuffers.resize ( swapChain.imageCount() );
		
		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].create ( device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT );
	}

	void	freeUniformBuffers ()
	{
		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].clean ();
	}

	void	createDescriptorSets ()
	{
		descriptorSets.resize ( swapChain.imageCount () );

			// create descriptors for last pass
		for ( uint32_t i = 0; i < swapChain.imageCount (); i++ )
		{
			descriptorSets  [i]
				.setLayout ( device, descAllocator, pipeline.getDescLayout () )
				.addBuffer ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffers [i], 0, sizeof ( Ubo) )
				.addImage  ( 1, fb.getAttachment ( 0 ), sampler )
				.addImage  ( 2, fb.getAttachment ( 1 ), sampler )
				.create    ();
		}		
			// create descriptors for rendering boxes 
		offscreenDescriptorSet1
			.setLayout ( device, descAllocator, offscreenPipeline.getDescLayout () )
			.addBuffer ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffers [0], 0, sizeof ( Ubo ) )
			.addImage  ( 1, decalMap, sampler )
			.addImage  ( 2, bump1,    sampler )
			.create    ();
		
		offscreenDescriptorSet2
			.setLayout ( device, descAllocator, offscreenPipeline.getDescLayout () )
			.addBuffer ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffers [0], 0, sizeof ( Ubo ) )
			.addImage  ( 1, stoneMap, sampler )
			.addImage  ( 2, bump2,    sampler )
			.create    ();
		
		offscreenDescriptorSet3
			.setLayout ( device, descAllocator, offscreenPipeline.getDescLayout () )
			.addBuffer ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffers [0], 0, sizeof ( Ubo ) )
			.addImage  ( 1, knotMap, sampler )
			.addImage  ( 2, bump2,   sampler )
			.create    ();
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		screen.setVertexAttrs ( pipeline )
				.setDevice         ( device )
				.setVertexShader   ( "shaders/ds-3-2.vert.spv" )
				.setFragmentShader ( "shaders/ds-3-2.frag.spv" )
				.setSize           ( swapChain.getExtent ().width, swapChain.getExtent ().height )
				.addVertexBinding  ( sizeof ( float ) * 4, 0, VK_VERTEX_INPUT_RATE_VERTEX )
				.addDescLayout     ( 0, DescSetLayout ()
							.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT )
							.add ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )		// color attachment 0
							.add ( 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )	)	// color attachment 1
				.setCullMode       ( VK_CULL_MODE_NONE )
				.setFrontFace      ( VK_FRONT_FACE_COUNTER_CLOCKWISE )
				.setDepthTest      ( true )
				.setDepthWrite     ( true )
				.create            ( renderPass );
			
		offscreenPipeline
				.setDevice         ( device )
				.setVertexShader   ( "shaders/ds-3-1.vert.spv" )
				.setFragmentShader ( "shaders/ds-3-1.frag.spv" )
				.setSize           ( swapChain.getExtent ().width, swapChain.getExtent ().height )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes<BasicVertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
							.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT	)
							.add ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT ) 		// decal
							.add ( 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT ) )		// bump
				.setCullMode       ( VK_CULL_MODE_NONE )
				.setFrontFace      ( VK_FRONT_FACE_COUNTER_CLOCKWISE )
				.setDepthTest      ( true )
				.setDepthWrite     ( true )
				.create            ( fb.getRenderpass () );

				// create before command buffers
		swapChain.createFramebuffers ( renderPass, depthTexture.getImageView () );

		createDescriptorSets         ();
		createCommandBuffers         ( renderPass );
		createOffscreenCommandBuffer ();
	}

	virtual	void	freePipelines () override
	{
		commandBuffers.clear ();	
		pipeline.clean       ();
		renderPass.clean     ();
		freeUniformBuffers   ();
		descriptorSets.clear ();
		descAllocator.clean  ();
	}
	
	virtual	void	submit ( uint32_t imageIndex ) override 
	{
		updateUniformBuffer ( imageIndex );

		VkFence					currentFence  = swapChain.currentInFlightFence ();

		vkResetFences ( device.getDevice (), 1, &currentFence );

		SubmitInfo ()
				.wait    ( { { swapChain.currentAvailableSemaphore (), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT } } )
				.buffers ( { offscreenCmd } )
				.signal  ( { offscreenSemaphore.getHandle () } )
				.submit  ( device.getGraphicsQueue (), swapChain.currentInFlightFence () );

		SubmitInfo ()
			.wait    ( { { offscreenSemaphore.getHandle (), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT } } )
			.buffers ( { commandBuffers [imageIndex] } )
			.signal  ( { swapChain.currentRenderFinishedSemaphore () } )
			.submit  ( device.getGraphicsQueue () );
	}

	void	createCommandBuffers ( Renderpass& renderPass )
	{
		auto	framebuffers = swapChain.getFramebuffers ();

		commandBuffers = device.allocCommandBuffers ( (uint32_t)framebuffers.size ());

		for ( size_t i = 0; i < commandBuffers.size(); i++ )
		{
			commandBuffers [i].begin ().beginRenderPass ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( swapChain.getExtent ().width, swapChain.getExtent ().height ).clearColor ().clearDepthStencil () )
				.pipeline          ( pipeline )
				.addDescriptorSets ( { descriptorSets[i] } )
				.setViewport       ( swapChain.getExtent () )
				.setScissor        ( swapChain.getExtent () )
				.render            ( &screen )
				.end               ();
		}
	}

	void	createOffscreenCommandBuffer ()
	{
		offscreenSemaphore.create ( device );
		offscreenCmd.create       ( device );

		offscreenCmd.begin ( true ).beginRenderPass ( RenderPassInfo ( fb.getRenderpass() )
				.clearColor ( 0, 0, 0, 1 ).clearColor ( 0, 0, 0, 1 ).clearDepthStencil ()
				.framebuffer ( fb ).extent ( fb.getWidth (), fb.getHeight () ) )
			.pipeline          ( offscreenPipeline )
			.setViewport       ( fb.getWidth (), fb.getHeight () )
			.setScissor        ( fb.getWidth (), fb.getHeight () )
			.addDescriptorSets ( { offscreenDescriptorSet1 } ).render ( box1 )
			.addDescriptorSets ( { offscreenDescriptorSet2 } ).render ( box2 ).render ( box3 ).render ( box4 ).render ( box5 )
			.addDescriptorSets ( { offscreenDescriptorSet3 } ).render ( knot )
			.end               ();
	}
	
	void updateUniformBuffer ( uint32_t currentImage )
	{
		auto				mv   = controller->getModelView  ();

		uniformBuffers [currentImage]->mv   = mv;
		uniformBuffers [currentImage]->proj = controller->getProjection ();
		uniformBuffers [currentImage]->nm  = normalMatrix ( mv );
	}
	
	virtual	void	idle () override 
	{
		double	t  = getTime ();

		controller->timeElapsed ( 30.0f*(t - time) );

		time = t;
	}

	void	createGeometry ()
	{
			// create meshes
		box1 = createBox  ( device, glm::vec3 ( -6, -0.1, -6 ),   glm::vec3 ( 12, 3, 12 ), nullptr, true );
		box2 = createBox  ( device, glm::vec3 ( -1.5, 0, -0.5 ),  glm::vec3 ( 1,  2,  2 ) );
		box3 = createBox  ( device, glm::vec3 ( 1.5, 0, -0.5 ),   glm::vec3 ( 1,  1,  1 ) );
		box4 = createBox  ( device, glm::vec3 ( -4, 0, -0.5 ),    glm::vec3 ( 1,  1,  1 ) );
		box5 = createBox  ( device, glm::vec3 ( -4, 0, -4 ),      glm::vec3 ( 1,  1,  1 ) ); 
		knot = createKnot ( device, 1, 4, 120, 30 );
	}
};

int main ( int argc, const char * argv [] ) 
{
	return DeferredWindow ( 1200, 900, "Deferred rendering in Vulkan" ).run ();
}
