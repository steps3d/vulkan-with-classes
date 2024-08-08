#include	<ctype.h>
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
};

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	std::vector<Texture>			textures;
	Sampler							sampler;
	std::unique_ptr<Mesh>           mesh;
	Buffer							buffer;

public:
	ExampleWindow ( int w, int h, const std::string& t, bool depth, DevicePolicy * p ) : VulkanWindow ( w, h, t, depth, p )
	{
		setController ( new RotateController ( this, glm::vec3(2.0f, 2.0f, 2.0f) ) );

		mesh = std::unique_ptr<Mesh> ( createKnot ( device, 1.0f, 0.8f, 120, 30 ) );

		descAllocator.setFlags ( VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT  );
		createTextures  ();
		sampler.create  ( device );		// use default options
		createPipelines ();
	}

	void	createTextures ()
	{
		std::string					texPath = "../../Textures/";
		std::vector<const char *>	texs    = { "Fieldstone.dds", "16.jpg", "flower.png", "lena.png", "block.jpg", "brick.tga", "oak.jpg", "rockwall.jpg" };

		textures.resize ( texs.size () );
		for ( size_t i = 0; i < texs.size (); i++ )
			if ( !textures [i].load ( device, texPath + texs [i] ) )
				fatal () << "Error loading texture " << texs [i] << std::endl;

	}

	void	createUniformBuffers ()
	{
		uniformBuffers.resize ( swapChain.imageCount() );
		
		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].create ( device );
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
				.addSampler       ( 1, sampler )
				.addImageArray    ( 2, textures )
				.create           ();
		}
	}
	
	virtual	void	createPipelines () override 
	{
		VkDescriptorBindingFlags descFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;	//VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		DescSetLayout	layout;

		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		layout
			.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT )
			.add ( 1, VK_DESCRIPTOR_TYPE_SAMPLER,		 VK_SHADER_STAGE_FRAGMENT_BIT )
			.add ( 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_SHADER_STAGE_FRAGMENT_BIT, uint32_t ( textures.size () ) )
			.addFlags ( { 0, 0, descFlags } )
			.create ( device.getDevice (), VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT );

		pipeline.setDevice ( device )
				.setVertexShader   ( "shaders/shader-bindless.vert.spv" )
				.setFragmentShader ( "shaders/shader-bindless.frag.spv" )
				.setSize           ( swapChain.getExtent () )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes <BasicVertex> ()
				.addDescLayout     ( 0, layout )
			.setCullMode       ( VK_CULL_MODE_NONE)
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
				.beginRenderPass   ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( swapChain.getExtent ().width, swapChain.getExtent ().height ).clearColor (0,0,0,1).clearDepthStencil () )
				.pipeline          ( pipeline )
				.addDescriptorSets ( { descriptorSets[i] } )
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
	DevicePolicy								policy;
	VkPhysicalDeviceDescriptorIndexingFeatures	featuresIndexing = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES   };

	featuresIndexing.shaderSampledImageArrayNonUniformIndexing    = VK_TRUE;
	featuresIndexing.runtimeDescriptorArray                       = VK_TRUE;
	featuresIndexing.shaderSampledImageArrayNonUniformIndexing    = VK_TRUE;
	featuresIndexing.runtimeDescriptorArray                       = VK_TRUE;
	featuresIndexing.descriptorBindingVariableDescriptorCount     = VK_TRUE;
	featuresIndexing.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	featuresIndexing.descriptorBindingUpdateUnusedWhilePending    = VK_TRUE;
	featuresIndexing.descriptorBindingPartiallyBound              = VK_TRUE;
		             
	policy.addFeatures ( &featuresIndexing );

	return ExampleWindow ( 800, 600, "Vulkan bindless", true,  &policy ).run ();
}
