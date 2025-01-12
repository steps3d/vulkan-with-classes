#include	<memory>
#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Mesh.h"
#include	"Controller.h"

struct Ubo 
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 offs;
	glm::vec4 color;
//	glm::mat3 nm;
};

struct PushData
{
	glm::vec4	offset;
	glm::vec4	color;
};

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	Texture							texture;
	Sampler							sampler;
	std::unique_ptr<Mesh>           mesh;

public:
	ExampleWindow ( int w, int h, const std::string& t ) : VulkanWindow ( w, h, t )
	{
		setController ( new RotateController ( this, glm::vec3(2.0f, 2.0f, 2.0f) ) );

		mesh = std::unique_ptr<Mesh> ( loadMesh ( device, "../../Models/teapot.3ds", 0.04f ) );

		sampler.create  ( device );		// use default optiona		
		texture.load    ( device, "../../Textures/Fieldstone.dds", false );
		createPipelines ();
	}

	void	createUniformBuffers ()
	{
		auto	size  = sizeof ( Ubo );
		auto	align = device.getProperties ().properties.limits.minUniformBufferOffsetAlignment;

		size = align * ( (size + align - 1) / align );
		uniformBuffers.resize ( swapChain.imageCount() );
		
		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].create ( device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 64, int ( align ) );			// each buffer for 64 Ubo structs
	}

	void	freeUniformBuffers ()
	{
		uniformBuffers.clear ();
	}

	void	createDescriptorSets ()
	{
		descriptorSets.resize ( swapChain.imageCount () );

		for ( uint32_t i = 0; i < swapChain.imageCount (); i++ )
		{
			descriptorSets  [i]
				.setLayout        ( device, descAllocator, pipeline.getDescLayout () )
				.addBuffer        ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, uniformBuffers [i], 0, sizeof ( Ubo ) )
				.addImage         ( 1, texture, sampler )
				.create           ();
		}
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		pipeline.setDevice ( device )
				.setVertexShader   ( "shaders/shader-dynamic-uniform.vert.spv" )
				.setFragmentShader ( "shaders/shader-dynamic-uniform.frag.spv" )
				.setSize           ( swapChain.getExtent ().width, swapChain.getExtent ().height )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes <BasicVertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
					.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,         VK_SHADER_STAGE_VERTEX_BIT )
					.add ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT ) )
			.setCullMode       ( VK_CULL_MODE_NONE               )
			.setDepthTest      ( true )
			.setDepthWrite     ( true )
			.create            ( renderPass );			

				// create before command buffers
		swapChain.createFramebuffers ( renderPass, depthTexture.getImageView () );

		createDescriptorSets ();
		createCommandBuffers ( renderPass );
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
		defaultSubmit       ( commandBuffers [imageIndex] );
	}

	void	createCommandBuffers ( Renderpass& renderPass )
	{
		auto	framebuffers = swapChain.getFramebuffers ();

		commandBuffers = device.allocCommandBuffers ( (uint32_t)framebuffers.size ());

		for ( size_t i = 0; i < commandBuffers.size(); i++ )
		{
			commandBuffers [i]
				.begin             ()
				.beginRenderPass   ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( swapChain.getExtent ().width, swapChain.getExtent ().height ).clearColor ().clearDepthStencil () )
				.pipeline          ( pipeline )
				.setViewport       ( swapChain.getExtent () )
				.setScissor        ( swapChain.getExtent () );

			for ( int j = 0; j < 64; j++ )
			{
				commandBuffers [i].addDescriptorSets ( { descriptorSets[i] }, { uniformBuffers[i].offsForItem ( j  ) } ).render ( mesh.get () );
			}

			commandBuffers [i].end ();
		}
	}

	void updateUniformBuffer ( uint32_t currentImage )
	{
		for ( int j = 0; j < 64; j++ )
		{
			uniformBuffers [currentImage][j].model = controller->getModelView  ();
			uniformBuffers [currentImage][j].view  = glm::mat4 ( 1 );
			uniformBuffers [currentImage][j].proj  = controller->getProjection ();
			uniformBuffers [currentImage][j].offs  = glm::vec4 ( j % 8 - 4, j / 8 - 4, 0, 0 );
			uniformBuffers [currentImage][j].color = glm::vec4 ( (j / 8) / 8.0f, 1 - (j%8) / 8.0f, 0, 1 );
		}
	}
};

int main ( int argc, const char * argv [] ) 
{
	return ExampleWindow ( 800, 600, "Simple mesh" ).run ();
}
