#include	<memory>
#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Mesh.h"
#include	"Controller.h"

struct Ubo 
{
	glm::mat4	mv;
	glm::mat4	nm;
	glm::mat4	proj;
	glm::vec4	eye;
	glm::vec4	lightDir;
	int			detail;
	float		droop;
	int			length;
	float		step;
};

class	GeometryShaderWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	GraphicsPipeline				pipelineHair, pipelineSolid;
	Renderpass						renderPass;
	Texture							texture;
	Sampler							sampler;
	std::unique_ptr<Mesh>			mesh;
	int								length = 1;

public:
	GeometryShaderWindow ( int w, int h, const std::string& t, bool hasDepth, DevicePolicy * p ) : VulkanWindow ( w, h, t, hasDepth, p )
	{
		setController ( new RotateController ( this, glm::vec3(-7, 0, 0) ) );

		mesh = std::unique_ptr<Mesh> ( loadMesh ( device, "../../Models/dragon.obj", 20 ) );

		sampler.create  ( device );		// use default options
		texture.load    ( device, "textures/texture.jpg", false );
		createPipelines ();
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
				.setLayout        ( device, descAllocator, pipelineHair.getDescLayout () )
				.addUniformBuffer ( 0, uniformBuffers [i], 0, sizeof ( Ubo ) )
				.addImage         ( 1, texture, sampler )
				.create           ();
		}
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		pipelineHair.setDevice ( device )
				.setVertexShader   ( "shaders/hedgehog.vert.spv" )
				.setFragmentShader ( "shaders/hedgehog.frag.spv" )
				.setGeometryShader ( "shaders/hedgehog.geom.spv" )
				.setSize           ( swapChain.getExtent () )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes <BasicVertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
					.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT )
					.add ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT ) )
				.setCullMode       ( VK_CULL_MODE_NONE               )
			.setDepthTest      ( true )
			.setDepthWrite     ( true )
			.create            ( renderPass );			

		pipelineSolid.setDevice ( device )
			.setVertexShader   ( "shaders/blinn-phong.vert.spv" )
			.setFragmentShader ( "shaders/blinn-phong.frag.spv" )
			.setSize           ( swapChain.getExtent () )
			.addVertexBinding  ( sizeof ( BasicVertex ) )
			.addVertexAttributes <BasicVertex> ()
			.addDescLayout     ( 0, DescSetLayout ()
				.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT )
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
		pipelineHair.clean   ();
		pipelineSolid.clean  ();
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
				.pipeline          ( pipelineSolid )
				.addDescriptorSets ( { descriptorSets[i] } )
				.setViewport       ( swapChain.getExtent () )
				.setScissor        ( swapChain.getExtent () )
				.render            ( mesh.get () )
				.pipeline          ( pipelineHair )
				.addDescriptorSets ( { descriptorSets[i] } )
				.render            ( mesh.get () )
				.end               ();
		}
	}

	void updateUniformBuffer ( uint32_t currentImage )
	{
		uniformBuffers [currentImage]->mv       = controller->getModelView  () * glm::translate(glm::mat4(1), -mesh->getBox().getCenter ());
		uniformBuffers [currentImage]->proj     = controller->getProjection ();
		uniformBuffers [currentImage]->nm       = normalMatrix ( uniformBuffers [currentImage]->mv );
		uniformBuffers [currentImage]->detail   = 1;
		uniformBuffers [currentImage]->droop    = 0.0003f;
		uniformBuffers [currentImage]->step     = 0.03f;
		uniformBuffers [currentImage]->length   = length;
		uniformBuffers [currentImage]->eye      = glm::vec4 ( controller->getEye (), 1);
		uniformBuffers [currentImage]->lightDir = glm::vec4 ( -1, 1, 1, 0 );
	}

	void	keyTyped ( int key, int scancode, int action, int mods )
	{
		VulkanWindow::keyTyped ( key, scancode, action, mods );

		if ( key == 61 )
			length++;
		else
		if ( key == 45 && length > 0 )
			length--;
	}
};

int main ( int argc, const char * argv [] ) 
{
	DevicePolicy	policy;

	policy.features.features.geometryShader = VK_TRUE;

	return GeometryShaderWindow ( 800, 600, "Geometry shader window", true, &policy ).run ();
}
