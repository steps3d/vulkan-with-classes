//
// Using textures with VulkanWindow
//

#include	<memory>
#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Texture.h"
#include	"Mesh.h"
#include	"Framebuffer.h"
#include	"Semaphore.h"
#include	"Controller.h"

struct Ubo
{
	glm::mat4 mv;
	glm::mat4 proj;
	glm::mat4 nm;
	glm::vec4 light;
	glm::mat4 shadowMat;

};

class	ShadowWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	std::vector<Uniform<Ubo>>		shadowUniformBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	Sampler							sampler;
	Framebuffer						fb;					// G-buffer 
	CommandBuffer					shadowCmd;
	DescriptorSet					offscreenDescriptorSet;
	Semaphore						offscreenSemaphore;
	GraphicsPipeline				shadowPipeline;
	float							zMin = 0.1f;
	float							zMax = 100.0f;	
	double							time = 0;
	glm::vec3						light = glm::vec3 ( 7, 0, 7 );
	std::unique_ptr<Mesh>			mesh1, mesh2;
	Texture							decalMap, stoneMap;
	Texture							shadowMap;
	const int						shadowMapSize = 1024;

public:
	ShadowWindow ( int w, int h, const std::string& t ) : VulkanWindow ( w, h, t, true )
	{
		//sampler.setMinFilter ( VK_FILTER_LINEAR ).setMagFilter ( VK_FILTER_LINEAR ).create ( device );
		sampler.setMipmapMode ( VK_SAMPLER_MIPMAP_MODE_NEAREST ).create ( device );

		decalMap.load ( device, "../../Textures/wood.png"  );
		stoneMap.load ( device, "../../Textures/brick.tga" );

			// create G-buffer with 2 RGBA16F color attachments and depth attachment
		fb.init ( device, shadowMapSize, shadowMapSize )
			.addAttachment ( VK_FORMAT_D24_UNORM_S8_UINT,  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		  .create ();
		  
			// create meshes
		mesh1 = std::unique_ptr<Mesh> ( createQuad ( device, glm::vec3 ( -6, -6, -2 ), glm::vec3 ( 12, 0, 0 ), glm::vec3 ( 0, 12, 0 ) ) );
		mesh2 = std::unique_ptr<Mesh> ( createKnot ( device, 1.0f, 0.7f, 90, 50 ) );
				
			// create all pipelines
		createPipelines ();
		setController   ( new RotateController ( this ) );
	}

	void	createUniformBuffers ()
	{
		uniformBuffers.resize       ( swapChain.imageCount() );
		shadowUniformBuffers.resize ( swapChain.imageCount() );

		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].create ( device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT );

		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			shadowUniformBuffers [i].create ( device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT );
	}

	void	freeUniformBuffers ()
	{
		for ( auto& it : uniformBuffers )
			it.clean ();
	}

	void	createDescriptorSets ()
	{
		descriptorSets.resize ( swapChain.imageCount () );

			// create descriptors for last pass
		for ( uint32_t i = 0; i < swapChain.imageCount (); i++ )
		{
			descriptorSets  [i]
				.setLayout ( device, descAllocator, pipeline.getDescLayout () )
				.addBuffer ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffers [i], 0, sizeof ( Ubo ) )
				.addImage  ( 2, fb.getAttachment ( 0 ), sampler )
				.addImage  ( 1, decalMap, sampler )
				.create    ();
		}		
			// create descriptors for rendering boxes 
		offscreenDescriptorSet
			.setLayout ( device, descAllocator, shadowPipeline.getDescLayout () )
			.addBuffer ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shadowUniformBuffers [0], 0, sizeof ( Ubo ) )
			.create    ();	
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();

		renderPass
			.addAttachment   ( swapChain.getFormat (),                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR )
			.addAttachment   ( depthTexture.getImage ().getFormat (), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
			.addSubpass      ( 0 )
			.addDepthSubpass ( 1 )
			.create          ( device );

		pipeline
				.setDevice         ( device )
				.setVertexShader   ( "shaders/shadow.vert.spv" )
				.setFragmentShader ( "shaders/shadow.frag.spv" )
				.setSize           ( swapChain.getExtent ().width, swapChain.getExtent ().height )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes<BasicVertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
						.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT )
						.add ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )		// depth texture
						.add ( 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )	)	// color texture
				.setCullMode       ( VK_CULL_MODE_NONE )
				.setFrontFace      ( VK_FRONT_FACE_COUNTER_CLOCKWISE )
				.setDepthTest      ( true )
				.setDepthWrite     ( true )
				.create            ( renderPass );
			
		shadowPipeline
				.setDevice         ( device )
				.setVertexShader   ( "shaders/depthpass.vert.spv" )
				.setFragmentShader ( "shaders/depthpass.frag.spv" )
				.setSize           ( shadowMapSize, shadowMapSize )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes<BasicVertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
						.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT	) )
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

		VkFence		currentFence  = swapChain.currentInFlightFence ();

		vkResetFences ( device.getDevice (), 1, &currentFence );

		SubmitInfo ()
			.wait    ( { { swapChain.currentAvailableSemaphore (), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT } } )
			.buffers ( { shadowCmd } )
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
				.render            ( mesh1.get () )
				.render            ( mesh2.get () )
				.end               ();
		}
	}

	void	createOffscreenCommandBuffer ()
	{
		offscreenSemaphore.create ( device );
		shadowCmd.create          ( device );

		shadowCmd.begin ( true ).beginRenderPass ( RenderPassInfo ( fb.getRenderpass() ).clearDepthStencil ().framebuffer ( fb ).extent ( fb.getWidth (), fb.getHeight () ) )
			.pipeline          ( shadowPipeline )
			.addDescriptorSets ( { offscreenDescriptorSet } )
			.setViewport       ( fb.getWidth (), fb.getHeight () )
			.setScissor        ( fb.getWidth (), fb.getHeight () )
			.render            ( mesh1.get () )
			.render            ( mesh2.get () )
			.end               ();
	}
	
	void updateUniformBuffer ( uint32_t currentImage )
	{
		auto	mv   = controller->getModelView  ();
		auto	proj = controller->getProjection ();
		Ubo		ubo  = {};

			// compute UB for shadow map
		ubo.proj        = projectionMatrix ( 90, 1.0f, 0.01f, 30.0f );
		ubo.shadowMat   = ubo.proj * glm::lookAt      ( light, glm::vec3 ( 0, 0, 0 ), glm::vec3 ( 0, 0, 1 ) );
		ubo.light       = glm::vec4 ( light, 1.0f );
		ubo.mv          = mv;
		ubo.proj        = ubo.shadowMat;

		shadowUniformBuffers [currentImage][0] = ubo;

			// compute UB for rendering
		ubo.mv          = mv;
		ubo.proj        = proj;
		ubo.nm          = normalMatrix ( ubo.mv );

		uniformBuffers [currentImage][0] = ubo;

	}
	
	virtual	void	idle () override 
	{
		double	t  = getTime ();

		controller->timeElapsed ( 30.0f*(t - time) );

		time = t;
	}
};

int main ( int argc, const char * argv [] ) 
{
	return ShadowWindow ( 1200, 900, "Shadow mapping in Vulkan" ).run ();
}
