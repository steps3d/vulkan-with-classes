//
// Using textures with VulkanWindow
//

#include	<memory>
#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Texture.h"
#include	"Mesh.h"

struct alignas(16) Ubo
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 eye;
	glm::vec4 lightDir;
	glm::mat4 nm;		// use mat3(ubo.nm)
};

class	PbrWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	Texture							albedo, metallic, normal, roughness;
	Sampler							sampler;
	std::unique_ptr<Mesh>           mesh;
	
public:
	PbrWindow ( int w, int h, const std::string& t ) : VulkanWindow ( w, h, t, true )
	{
		mesh = std::unique_ptr<Mesh> ( createKnot ( device, 1.0f, 0.7f, 120, 30 ) );
//		mesh = createBox ( device, glm::vec3 ( -1.0f ), glm::vec3 ( 2.0f ) );
//		mesh = loadMesh ( device, "../../Models/teapot.3ds", 0.04f );
				
		sampler.setMinFilter ( VK_FILTER_LINEAR ).setMagFilter ( VK_FILTER_LINEAR ).create ( device );
		albedo.   load ( device, "textures/rusted_iron/albedo.png",    true );
		metallic. load ( device, "textures/rusted_iron/metallic.png",  true );
		normal.   load ( device, "textures/rusted_iron/normal.png",    true );
		roughness.load ( device, "textures/rusted_iron/roughness.png", true );

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
				.addImage         ( 1, albedo,     sampler )
				.addImage         ( 2, metallic,   sampler )
				.addImage         ( 3, normal,     sampler )
				.addImage         ( 4, roughness,  sampler )
				.create           ();
		}
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers ();
		createDefaultRenderPass ( renderPass );

		pipeline
				.setDevice ( device )
				.setVertexShader   ( "shaders/pbr.vert.spv" )
				.setFragmentShader ( "shaders/pbr.frag.spv" )
				.setSize           ( swapChain.getExtent ().width, swapChain.getExtent ().height )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes <BasicVertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
							.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT   )
							.add ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )
							.add ( 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )
							.add ( 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )
							.add ( 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT ) )
				.setCullMode       ( VK_CULL_MODE_NONE               )
				.setFrontFace      ( VK_FRONT_FACE_COUNTER_CLOCKWISE )
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

	void	createCommandBuffers ( Renderpass& renderPass )			// size - swapChain.framebuffers.size ()
	{
		auto	framebuffers = swapChain.getFramebuffers ();

		commandBuffers = device.allocCommandBuffers ( (uint32_t)framebuffers.size () );

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
		float				time = (float)getTime ();

		uniformBuffers [currentImage]->model    = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		uniformBuffers [currentImage]->view     = glm::lookAt(glm::vec3(4.0f, 4.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		uniformBuffers [currentImage]->proj     = projectionMatrix ( 45, getAspect (), 0.1f, 10.0f );
		uniformBuffers [currentImage]->nm       = normalMatrix ( uniformBuffers [currentImage]->model );
		uniformBuffers [currentImage]->eye      = glm::vec4 ( 4.0f );
		uniformBuffers [currentImage]->lightDir = glm::vec4 ( 0.0f, 0.0f, 1.0f, 1.0f );
	}
};

int main ( int argc, const char * argv [] ) 
{
	return PbrWindow ( 800, 600, "PBR" ).run ();
}
