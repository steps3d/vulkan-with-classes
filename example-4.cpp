#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Model.h"
#include	"Controller.h"

struct UniformBufferObject 
{
	glm::mat4 model;
	glm::mat4 proj;
	glm::vec4 eye;
	glm::vec4 light;
};

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Buffer>				uniformBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	Model							model;
	Sampler							sampler;
	glm::vec3						light = glm::vec3 ( -7, 0, 0 );
	glm::vec3						eye   = glm::vec3 ( -7, 0, 0 );

public:
	ExampleWindow ( int w, int h, const std::string& t ) : VulkanWindow ( w, h, t )
	{
		setController ( new RotateController ( this, eye ) );

		model.load (device,  "models/FBX/ppsh/source/ppsh-41.fbx", "models/FBX/ppsh/textures",  "Ppsh-41" );
		sampler.create  ( device );		// use default options

		createPipelines ();
	}

	void	createUniformBuffers ()
	{
		uniformBuffers.resize ( swapChain.imageCount() );
		
		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].create ( device, sizeof ( UniformBufferObject ), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
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
				.setLayout      ( device, descAllocator, pipeline.getDescLayout () )
				.addBuffer      ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffers [i], 0, sizeof ( UniformBufferObject ) )
				.addSampler     ( 1, sampler )
				.addImageArray  ( 2, model.getTextures () )
				.create    ();
		}
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		pipeline.setDevice ( device )
				.setVertexShader   ( "shaders/model-pbr.vert.spv" )
				.setFragmentShader ( "shaders/model-pbr.frag.spv" )
				.setSize           ( swapChain.getExtent ().width, swapChain.getExtent ().height )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes <BasicVertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
					.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,    VK_SHADER_STAGE_VERTEX_BIT )
					.add ( 1, VK_DESCRIPTOR_TYPE_SAMPLER,           VK_SHADER_STAGE_FRAGMENT_BIT )
					.add ( 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,     VK_SHADER_STAGE_FRAGMENT_BIT, (uint32_t) model.getTextures ().size () ) )
				.addPushConstRange (  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof ( PushConstants ) )
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

		defaultSubmit ( commandBuffers [imageIndex] );
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
				.setScissor        ( swapChain.getExtent () );

			model.render ( pipeline, commandBuffers [i], glm::mat4 ( 1.0f ) );

			commandBuffers [i].end ();
		}
	}

	void updateUniformBuffer ( uint32_t currentImage )
	{
		float				time = (float)getTime ();
		UniformBufferObject ubo  = {};

		ubo.model = controller->getModelView  ();
		ubo.proj  = projectionMatrix ( 45, getAspect (), 0.1f, 250000.0f );
		ubo.eye   = glm::vec4 ( eye, 1.0f );
		ubo.light = glm::vec4 ( light, 1.0 );

		uniformBuffers [currentImage].copy ( &ubo, sizeof ( ubo ) );
	}
};

int main ( int argc, const char * argv [] ) 
{
	return ExampleWindow ( 1200, 1200, "Test window" ).run ();
}
