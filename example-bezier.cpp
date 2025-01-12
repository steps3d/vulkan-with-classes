#include	<memory>
#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Mesh.h"
#include	"Controller.h"

struct Ubo
{
	glm::mat4	mv;
	glm::mat4	proj;
	int			inner;
	int			outer;
};

struct Vertex 
{
	glm::vec3 pos;
};

template <>
GraphicsPipeline&	registerVertexAttrs<Vertex> ( GraphicsPipeline& pipeline ) 
{
	return pipeline
		.addVertexAttr ( 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) )	;	// binding, location, format, offset
}

#define	PATCH_SIZE	16							// 16 control points 
#define	VERTEX_SIZE	(3*sizeof(float))
#define	NUM_VERTICES	(PATCH_SIZE)

static const std::vector<float> vertices = //[PATCH_SIZE * 3] = 
{
	-2.0f, -2.0f, 0.0f, 		// P00*
	-1.0f, -2.0f, 1.0f, 		// P10
	 1.0f, -1.0f, 2.0f,			// P20
	 2.0f, -2.0f, 0.0f,			// P30*
	-2.0f, -1.0f, 1.0f,			// P01
	-1.0f, -1.0f, 1.0f,			// P11
	 2.0f,  0.0f, 1.0f,			// P21
	 2.0f, -1.0f, 2.0f,			// P31
	-3.0f,  0.0f, 1.0f,			// P02
	-1.0f, -1.5f, 1.0f,			// P12
	 0.0f,  0.0f, 0.0f,			// P22
	 1.0f,  1.0f, 1.0f,			// P32
	-2.0f,  2.0f, 0.0f,			// P03*
	-1.5f,  3.0f, 2.0f,			// P13
	 1.0f,  3.0f, 2.0f,			// P23
	 2.0f,  2.0f, 0.0f			// P33*
};

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	Buffer							vertexBuffer;
	int								inner = 2, outer = 2;

public:
	ExampleWindow ( int w, int h, const std::string& t, DevicePolicy * p ) : VulkanWindow ( w, h, t, true, p )
	{
		setController ( new RotateController ( this, glm::vec3(2.0f, 2.0f, 2.0f) ) );

		vertexBuffer.create ( device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, Buffer::hostWrite );
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
				.create           ();
		}
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		pipeline.setDevice ( device )
				.setVertexShader      ( "shaders/bezier.vert.spv" )
				.setFragmentShader    ( "shaders/bezier.frag.spv" )
				.setTessControlShader ( "shaders/bezier.tesc.spv" )
				.setTessEvalShader    ( "shaders/bezier.tese.spv" )
				.setPolygonMode       ( VK_POLYGON_MODE_LINE )
				.setTopology          ( VK_PRIMITIVE_TOPOLOGY_PATCH_LIST )
				.setPatchSize         ( 16 )
				.setSize              ( swapChain.getExtent ().width, swapChain.getExtent ().height )
				.addVertexBinding     ( sizeof ( Vertex ) )
				.addVertexAttributes <BasicVertex> ()
				.addDescLayout       ( 0, DescSetLayout ()
					.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT ) )
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
				.bindVertexBuffers ( { {vertexBuffer, 0} } )
				.addDescriptorSets ( { descriptorSets[i] } )
				.setViewport       ( swapChain.getExtent () )
				.setScissor        ( swapChain.getExtent () )
				.draw              ( 16 )
				.end               ();
		}
	}

	void updateUniformBuffer ( uint32_t currentImage )
	{
		uniformBuffers [currentImage]->mv    = controller->getModelView  ();
		uniformBuffers [currentImage]->proj  = controller->getProjection ();
		uniformBuffers [currentImage]->inner = inner;
		uniformBuffers [currentImage]->outer = outer;
	}

	virtual	void	keyTyped    ( int key, int scancode, int action, int mods ) override
	{
		if ( action == GLFW_RELEASE )
		{
			if ( key == 'A' )
				inner++;
			else
			if ( key == 'D' && inner > 0 )
				inner--;
			else
			if ( key == 'W' )
				outer++;
			else
			if ( key == 'X' && outer > 0 )
				outer--;
		}

		VulkanWindow::keyTyped ( key, scancode, action, mods );
	}
};

int main ( int argc, const char * argv [] ) 
{
	DevicePolicy	policy;

	policy.features.features.tessellationShader = VK_TRUE;
	policy.features.features.fillModeNonSolid   = VK_TRUE;

	return ExampleWindow ( 800, 600, "Bezier patch", &policy ).run ();
}
