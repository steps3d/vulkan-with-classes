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
	glm::mat3 nm;
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
		uniformBuffers.resize ( swapChain.imageCount() );
		
		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].create ( device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT );
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
				.addUniformBuffer ( 0, uniformBuffers [i], 0, sizeof ( Ubo ) )
				.addImage         ( 1, texture, sampler )
				.create           ();
		}
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		pipeline.setDevice ( device )
				.setVertexShader   ( "shaders/shader-tex.vert.spv" )
				.setFragmentShader ( "shaders/shader-tex.frag.spv" )
				.setSize           ( swapChain.getExtent ().width, swapChain.getExtent ().height )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes <BasicVertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
					.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT )
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
				.addDescriptorSets ( { descriptorSets[i] } )
				.setViewport       ( swapChain.getExtent () )
				.setScissor        ( swapChain.getExtent () )
				.render            ( mesh.get () )
				.end               ();
		}
	}

	void updateUniformBuffer ( uint32_t currentImage )
	{
		uniformBuffers [currentImage]->model = controller->getModelView  ();
		uniformBuffers [currentImage]->view  = glm::mat4 ( 1 );
		uniformBuffers [currentImage]->proj  = controller->getProjection ();
	}
};

int main ( int argc, const char * argv [] ) 
{
	return ExampleWindow ( 800, 600, "Simple mesh" ).run ();
}
