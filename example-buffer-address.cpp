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
	glm::mat3 nm;
};

struct Instance
{
	glm::vec4	offset;
	glm::vec4	color;
};

struct BufferRec
{
	Instance	instances [64];
};

struct PushData
{
	uint64_t	bufferPtr;
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
	Buffer							buffer;

public:
	ExampleWindow ( int w, int h, const std::string& t, bool depth, DevicePolicy * p ) : VulkanWindow ( w, h, t, depth, p )
	{
		setController ( new RotateController ( this, glm::vec3(2.0f, 2.0f, 2.0f) ) );

		mesh = std::unique_ptr<Mesh> ( loadMesh ( device, "../../Models/teapot.3ds", 0.04f ) );

		sampler.create  ( device );		// use default options
		texture.load    ( device, "../../Textures/Fieldstone.dds", false );
		createBuffer    ();
		createPipelines ();
	}

	void	createBuffer ()
	{
		std::vector<Instance>	instances ( 64 );

		for ( int i = 0; i < 64; i++ )
		{
			instances [i].offset = glm::vec4 ( i % 8 - 4, i / 8 - 4, 0, 0 );
			instances [i].color  = glm::vec4 ( (i / 8) / 8.0f, 1 - (i % 8) / 8.0f, 0, 1 );
		}

		buffer.create ( device, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, instances, Buffer::hostWrite );
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
				.addImage         ( 1, texture, sampler )
				.create           ();
		}
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		pipeline.setDevice ( device )
				.setVertexShader   ( "shaders/shader-buffer-address.vert.spv" )
				.setFragmentShader ( "shaders/shader-buffer-address.frag.spv" )
				.setSize           ( swapChain.getExtent () )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes <BasicVertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
					.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT )
					.add ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT ) )
			.addPushConstRange (  VK_SHADER_STAGE_VERTEX_BIT, sizeof ( PushData ) )
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
				.beginRenderPass   ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( swapChain.getExtent () ).clearColor ().clearDepthStencil () )
				.pipeline          ( pipeline )
				.addDescriptorSets ( { descriptorSets[i] } );

			PushData	data = { buffer.getDeviceAddress () };

			commandBuffers [i]
				.pushConstants ( pipeline.getLayout (), VK_SHADER_STAGE_VERTEX_BIT, data )
				.renderInstanced   ( mesh.get (), 64 );		// draw 64 instances

			commandBuffers [i].end ();
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
	DevicePolicy					 policy;

	return ExampleWindow ( 800, 600, "Vulkan buffer address", true,  &policy ).run ();
}
