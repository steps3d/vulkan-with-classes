#include	<memory>
#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Mesh.h"
#include	"Controller.h"
#include	"StatisticsPool.h"
#include	"TimestampPool.h"

struct Ubo 
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat3 nm;
};

class	DynamicRenderingWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	Texture							texture;
	Sampler							sampler;
	std::unique_ptr<Mesh>           mesh;

	PFN_vkCmdBeginRenderingKHR	vkCmdBeginRenderingKHR {};
	PFN_vkCmdEndRenderingKHR	vkCmdEndRenderingKHR   {};

public:
	DynamicRenderingWindow ( int w, int h, const std::string& t, DevicePolicy * p ) : VulkanWindow ( w, h, t, true, p )
	{
		setController ( new RotateController ( this, glm::vec3(2.0f, 2.0f, 2.0f) ) );

		mesh = std::unique_ptr<Mesh> ( loadMesh ( device, "../../Models/teapot.3ds", 0.04f ) );

		sampler.create  ( device );		// use default optiona		
		texture.load    ( device, "../../Textures/Fieldstone.dds", false );
		loadExtensions  ();
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
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {};
		VkFormat						 swapChainFormats []         = { swapChain.getFormat () };

		pipelineRenderingCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		pipelineRenderingCreateInfo.colorAttachmentCount    = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = swapChainFormats;
		pipelineRenderingCreateInfo.depthAttachmentFormat   = depthTexture.getFormat ();
		pipelineRenderingCreateInfo.stencilAttachmentFormat = depthTexture.getFormat ();

		createUniformBuffers    ();

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
			.setNumColorBlendAttachments ( 1 )
			.addAddInfo        ( &pipelineRenderingCreateInfo )
			.create            ( renderPass );			

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
		//auto	framebuffers = swapChain.getFramebuffers ();

		commandBuffers = device.allocCommandBuffers ( swapChain.imageCount () );

		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
		{
			VkRenderingAttachmentInfoKHR colorAttachment        = {};
			VkRenderingAttachmentInfoKHR depthStencilAttachment = {};

			colorAttachment.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
			colorAttachment.imageView        = swapChain.getImageViews () [i];	
			colorAttachment.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachment.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.clearValue.color = { 0.0f,0.0f,0.0f,0.0f };

			depthStencilAttachment.sType                   = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
			depthStencilAttachment.imageView               = depthTexture.getImageView ();
			depthStencilAttachment.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthStencilAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthStencilAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
			depthStencilAttachment.clearValue.depthStencil = { 1.0f,  0 };

			VkRenderingInfoKHR renderingInfo = {};

			renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
			renderingInfo.renderArea           = { 0, 0, uint32_t ( width ), uint32_t ( height ) };
			renderingInfo.layerCount           = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments    = &colorAttachment;
			renderingInfo.pDepthAttachment     = &depthStencilAttachment;
			renderingInfo.pStencilAttachment   = &depthStencilAttachment;

			// With dynamic rendering there are no subpass dependencies, 
			// we need to take care of proper layout transitions by using barriers
			// for color and depth images

			auto	barrierImage = imageBarrier  ( swapChain.getImages () [i], 
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,		// srcStageMask
				0,													// srcAccessMask
				VK_IMAGE_LAYOUT_UNDEFINED,							// oldLayout
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,		// dstStageMask
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,				// dstAccessMask
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// newLayout
				VK_IMAGE_ASPECT_COLOR_BIT							// aspectMask
			);

			auto	barrierDepth = imageBarrier  ( depthTexture.getImage (),   
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,		// srcStageMask, 
				0,																							// srcAccessMask, 
				VK_IMAGE_LAYOUT_UNDEFINED,																	// oldLayout, 
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,		// dstStageMask, 
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,												// dstAccessMask, 
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,											// newLayout, 
				VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT										// aspectMask
			);


			commandBuffers [i].begin ();

			pipelineBarrier ( commandBuffers [i], { barrierImage, barrierDepth } );

			vkCmdBeginRenderingKHR ( commandBuffers [i].getHandle (), &renderingInfo );

			//VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			//vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			//VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			//vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			commandBuffers [i]
				.pipeline          ( pipeline )
				.addDescriptorSets ( { descriptorSets[i] } )
				.render            ( mesh.get () );

			vkCmdEndRenderingKHR ( commandBuffers [i].getHandle () );

			auto	barrierImage2 = imageBarrier  ( swapChain.getImages () [i], 
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,		// srcStageMask
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,				// srcAccessMask
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// oldLayout
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,				// dstStageMask
				0,													// dstAccessMask
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,					// newLayout
				VK_IMAGE_ASPECT_COLOR_BIT							// aspectMask
			);

			pipelineBarrier ( commandBuffers [i], { barrierImage2 } );

			commandBuffers [i].end ();
		}
	}

	void updateUniformBuffer ( uint32_t currentImage )
	{
		uniformBuffers [currentImage]->model = controller->getModelView  ();
		uniformBuffers [currentImage]->view  = glm::mat4 ( 1 );
		uniformBuffers [currentImage]->proj  = controller->getProjection ();
	}

	void	loadExtensions ()
	{
		vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr( device.getDevice (), "vkCmdBeginRenderingKHR" ) );
		vkCmdEndRenderingKHR   = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr  ( device.getDevice (), "vkCmdEndRenderingKHR" ) );
	}
};

int main ( int argc, const char * argv [] ) 
{
	DevicePolicy	policy;

	policy.addDeviceExtension ( VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME );

	return DynamicRenderingWindow ( 800, 600, "Dynamic rendering", &policy ).run ();
}
