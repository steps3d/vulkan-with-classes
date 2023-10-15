#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"

struct Ubo
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct Vertex 
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
};

template <>
GraphicsPipeline&	registerVertexAttrs<Vertex> ( GraphicsPipeline& pipeline ) 
{
	return pipeline
		.addVertexAttr     ( 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) )
		.addVertexAttr     ( 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) )
		.addVertexAttr     ( 0, 2, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, texCoord) );
}

const std::vector<Vertex> vertices = 
{
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f},  {1.0f, 0.0f}},
	{{0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f},  {0.0f, 0.0f}},
	{{0.5f, 0.5f,   0.0f}, {0.0f, 0.0f, 1.0f},  {0.0f, 1.0f}},
	{{-0.5f, 0.5f,  0.0f}, {1.0f, 1.0f, 1.0f},  {1.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f,  -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f,   -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f,  -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = 
{
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	Buffer							vertexBuffer;
	Buffer							indexBuffer;
	Texture							texture;
	Sampler							sampler;

public:
	ExampleWindow ( int w, int h, const std::string& t ) : VulkanWindow ( w, h, t )
	{
		vertexBuffer.create  ( device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, Buffer::hostWrite );
		indexBuffer.create   ( device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  indices,  Buffer::hostWrite );
		sampler.setMinFilter ( VK_FILTER_LINEAR ).setMagFilter ( VK_FILTER_LINEAR ).create ( device );
		texture.load         ( device, "textures/texture.jpg", false, false );
		createPipelines      ();
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
				.setVertexShader   ( "shaders/shader.4.vert.spv" )
				.setFragmentShader ( "shaders/shader.4.frag.spv" )
				.setSize           ( swapChain.getExtent () )
				.addVertexBinding  ( sizeof ( Vertex ) )
				.addVertexAttributes <Vertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
					.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT )
					.add ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT ) )
				.setCullMode       ( VK_CULL_MODE_NONE               )
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
	
	void	createCommandBuffers ( Renderpass& renderPass )
	{
		auto	framebuffers = swapChain.getFramebuffers ();

		commandBuffers = device.allocCommandBuffers ( (uint32_t)framebuffers.size ());

		for ( size_t i = 0; i < commandBuffers.size(); i++ )
		{
			commandBuffers [i].begin ().beginRenderPass ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( swapChain.getExtent () ).clearColor ().clearDepthStencil () )
				.pipeline ( pipeline )
				.bindVertexBuffers ( { {vertexBuffer, 0} } )
				.bindIndexBuffer ( indexBuffer )
				.addDescriptorSets ( { descriptorSets[i] } )
				.drawIndexed ( static_cast<uint32_t>(indices.size()) )
				.end ();
		}
	}

	virtual	void	submit ( uint32_t imageIndex ) override 
	{
		updateUniformBuffer ( imageIndex );
		defaultSubmit       ( commandBuffers [imageIndex] );
	}

	void updateUniformBuffer ( uint32_t currentImage )
	{
		float	time = (float)getTime ();
		auto	fwd  = glm::vec3(0.0f, 0.0f, 1.0f);

		uniformBuffers [currentImage]->model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), fwd);
		uniformBuffers [currentImage]->view  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), fwd);
		uniformBuffers [currentImage]->proj  = projectionMatrix ( 45, getAspect (), 0.1f, 10 );
	}
};

int main ( int argc, const char * argv [] ) 
{
	return ExampleWindow ( 800, 600, "Test window" ).run ();
}
