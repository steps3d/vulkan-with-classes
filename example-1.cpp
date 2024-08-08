#include	"VulkanWindow.h"

struct Vertex 
{
	glm::vec2 pos;
	glm::vec3 color;
};

template <>
GraphicsPipeline&	registerVertexAttrs<Vertex> ( GraphicsPipeline& pipeline ) 
{
	return pipeline
		.addVertexAttr ( 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)   )		// binding, location, format, offset
		.addVertexAttr ( 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) );
}

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>	commandBuffers;
	std::vector<DescriptorSet> 	descriptorSets;
	GraphicsPipeline			pipeline;
	Renderpass					renderPass;
	Buffer						vertexBuffer;

public:
	ExampleWindow ( int w, int h, const std::string& t ) : VulkanWindow ( w, h, t )
	{
		const std::vector<Vertex> vertices = 
		{
			{ {0.0f, -0.5f},  {1.0f, 0.0f, 0.0f}},
			{ {0.5f,  0.5f},  {0.0f, 1.0f, 0.0f}},
			{{-0.5f,  0.5f},  {0.0f, 0.0f, 1.0f}}
		};

		vertexBuffer.create ( device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, Buffer::hostWrite );

		createPipelines ();		// we're not going to inherit from this
	}

	virtual	void	createPipelines () override 
	{
			// default renderPass
		createDefaultRenderPass ( renderPass );

		pipeline
			.setDevice ( device )
			.setVertexShader   ( "shaders/shader.2.vert.spv" )
			.setFragmentShader ( "shaders/shader.2.frag.spv" )
			.setSize           ( swapChain.getExtent () )
			.addVertexBinding  ( sizeof ( Vertex ) )
			.addVertexAttributes <Vertex> ()
			.addDescLayout     ( 0, DescSetLayout () )
			.create            ( renderPass );

			// create before command buffers
		swapChain.createFramebuffers ( renderPass, depthTexture.getImageView () );		// m.b. depthTexture instead of getImageView ???

		createDescriptorSets ();
		createCommandBuffers ( renderPass );
	}

	virtual	void	freePipelines () override
	{
		commandBuffers.clear ();
		pipeline.clean       ();
		renderPass.clean     ();
		descriptorSets.clear ();
		descAllocator.clean  ();
	}

	virtual	void	submit ( uint32_t imageIndex ) override 
	{
		defaultSubmit ( commandBuffers [imageIndex] );
	}

	void	createDescriptorSets ()
	{
		descriptorSets.resize ( swapChain.imageCount () );

		for ( auto& desc : descriptorSets )
			desc.setLayout ( device, descAllocator, pipeline.getDescLayout () ).create ();
	}

	void	createCommandBuffers ( Renderpass& renderPass )
	{
		auto&	framebuffers = swapChain.getFramebuffers ();
		auto	size         = framebuffers.size ();

		commandBuffers = device.allocCommandBuffers ( (uint32_t)size );

		for ( size_t i = 0; i < size; i++ )
			commandBuffers [i].begin ().beginRenderPass ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( swapChain.getExtent () ).clearColor ( 0, 0, 0, 1 ).clearDepthStencil () )
				.pipeline          ( pipeline )
				.bindVertexBuffers (  { {vertexBuffer, 0} } )
				.addDescriptorSets ( {descriptorSets[i]} )
				.draw              ( 3 )
				.end               ();
	}
};

int main ( int argc, char * argv [] )
{
	return ExampleWindow ( 800, 600, "Vulkan example" ).run ();
}
