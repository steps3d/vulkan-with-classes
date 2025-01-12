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
	glm::mat4	nm;
};

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	std::vector<Texture>			texture;
	Sampler							sampler;
	std::unique_ptr<Mesh>			mesh;

public:
	ExampleWindow ( int w, int h, const std::string& t ) : VulkanWindow ( w, h, t ), texture ( 8 )
	{
		setController ( new RotateController ( this, glm::vec3(-7, 0, 0) ) );

		mesh = std::unique_ptr<Mesh> ( createKnot  ( device, 1.0f, 0.7f, 120, 30 ) );

		sampler.create  ( device );		// use default optiona		

		std::string	texPath = "../../Textures/";
		std::vector<const char *>	texs = { "Fieldstone.dds", "16.jpg", "flower.png", "lena.png", "block.jpg", "brick.tga", "oak.jpg", "rockwall.jpg" };

		for ( size_t i = 0; i < texs.size (); i++ )
			if ( !texture [i].load ( device, texPath + texs [i] ) )
				fatal () << "Error loading texture " << texs [i] << std::endl;

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
				.addUniformBuffer ( 0, uniformBuffers [i], 0, uniformBuffers [i].size () )
				.addSampler       ( 1, sampler )
				.addImageArray    ( 2, texture )
				.create           ();
		}
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		pipeline.setDevice ( device )
			.setVertexShader   ( "shaders/tex-array.vert.spv" )
			.setFragmentShader ( "shaders/tex-array.frag.spv" )
			.setSize           ( swapChain.getExtent ().width, swapChain.getExtent ().height )
			.addVertexBinding  ( sizeof ( BasicVertex ) )
			.addVertexAttributes <BasicVertex> ()
			.addDescLayout     ( 0, DescSetLayout ()
				.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  VK_SHADER_STAGE_VERTEX_BIT )
				.add ( 1, VK_DESCRIPTOR_TYPE_SAMPLER,         VK_SHADER_STAGE_FRAGMENT_BIT )
				.add ( 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,   VK_SHADER_STAGE_FRAGMENT_BIT, 8 ) )
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
				.beginRenderPass   ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( swapChain.getExtent ().width, swapChain.getExtent ().height ).clearColor (0,0,0,1).clearDepthStencil () )
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
		auto	mv = controller->getModelView  () * glm::translate(glm::mat4(1), -mesh->getBox().getCenter ());

		uniformBuffers [currentImage]->mv   = mv;
		uniformBuffers [currentImage]->proj = controller->getProjection ();
		uniformBuffers [currentImage]->nm   = normalMatrix ( mv );
	}
};

int main ( int argc, const char * argv [] ) 
{
	return ExampleWindow ( 800, 600, "Test window" ).run ();
}
