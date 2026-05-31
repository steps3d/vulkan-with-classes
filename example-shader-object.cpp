#include	<memory>
#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Mesh.h"
#include	"Controller.h"
#include	"StatisticsPool.h"
#include	"TimestampPool.h"

/*
class	ShaderObject
{
	VkShaderEXT	shader {};
	VkDevice	device {};

public:
	ShaderObject  () = default;
	~ShaderObject ()
	{
		if ( shader )
			vkDestroyShaderEXT ( device, shader, nullptr );
	}

	bool	create ( Device& dev, Data& data, VkShaderStageFlagBits stage, DescSetLayout * layout = nullptr, const std::string& name = "main" )
	{
		VkShaderCreateInfoEXT shaderCreateInfo {};
		VkDescriptorSetLayout	descSetLayout = layout -> getHandle ();

		device                     = dev.getDevice ();
		shaderCreateInfo.sType     = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
		shaderCreateInfo.flags     = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
		shaderCreateInfo.stage     = stage;
		//shaderCreateInfo.nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderCreateInfo.codeType  = VK_SHADER_CODE_TYPE_BINARY_EXT;
		shaderCreateInfo.pCode     = reinterpret_cast<const uint32_t*>( data.getPtr () );
		shaderCreateInfo.codeSize  = data.getLength ();
		shaderCreateInfo.pName     = name.c_str ();

		if ( layout != nullptr )
		{
			shaderCreateInfo.setLayoutCount = 1;
			shaderCreateInfo.pSetLayouts    = &descSetLayout;
		}

		if ( vkCreateShadersEXT ( device, 1, &shaderCreateInfo, nullptr, &shader ) != VK_SUCCESS )
		{
			warning () << "Error creatinh shjader object " << data.getFileName () << std::endl;

			return false;
		}

		return true;
	}
};
*/

struct Ubo 
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat3 nm;
};

class	ShaderObjectWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	DescSetLayout					descSetLayout;
	VkPipelineLayout				pipelineLayout = VK_NULL_HANDLE;
	//ShaderObject					vertexShader;
	//ShaderObject					fragmentShader;
	VkShaderEXT						vertexShader   {};
	VkShaderEXT						fragmentShader {};


	Texture							texture;
	Sampler							sampler;
	std::unique_ptr<Mesh>           mesh;

	PFN_vkCreateShadersEXT       vkCreateShadersEXT       { VK_NULL_HANDLE };
	PFN_vkDestroyShaderEXT       vkDestroyShaderEXT       { VK_NULL_HANDLE };
	PFN_vkCmdBindShadersEXT      vkCmdBindShadersEXT      { VK_NULL_HANDLE };
	PFN_vkGetShaderBinaryDataEXT vkGetShaderBinaryDataEXT { VK_NULL_HANDLE };

		// VK_EXT_shader_objects requires render passes to be dynamic
	PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR   { VK_NULL_HANDLE };
	PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR       { VK_NULL_HANDLE };

		// With VK_EXT_shader_object pipeline state must be set at command buffer creation using these functions
	PFN_vkCmdSetAlphaToCoverageEnableEXT   vkCmdSetAlphaToCoverageEnableEXT   { VK_NULL_HANDLE };
	PFN_vkCmdSetColorBlendEnableEXT        vkCmdSetColorBlendEnableEXT        { VK_NULL_HANDLE };
	PFN_vkCmdSetColorWriteMaskEXT          vkCmdSetColorWriteMaskEXT          { VK_NULL_HANDLE };
	PFN_vkCmdSetCullModeEXT                vkCmdSetCullModeEXT                { VK_NULL_HANDLE };
	PFN_vkCmdSetDepthBiasEnableEXT         vkCmdSetDepthBiasEnableEXT         { VK_NULL_HANDLE };
	PFN_vkCmdSetDepthCompareOpEXT          vkCmdSetDepthCompareOpEXT          { VK_NULL_HANDLE };
	PFN_vkCmdSetDepthTestEnableEXT         vkCmdSetDepthTestEnableEXT         { VK_NULL_HANDLE };
	PFN_vkCmdSetDepthWriteEnableEXT        vkCmdSetDepthWriteEnableEXT        { VK_NULL_HANDLE };
	PFN_vkCmdSetFrontFaceEXT               vkCmdSetFrontFaceEXT               { VK_NULL_HANDLE };
	PFN_vkCmdSetPolygonModeEXT             vkCmdSetPolygonModeEXT             { VK_NULL_HANDLE };
	PFN_vkCmdSetPrimitiveRestartEnableEXT  vkCmdSetPrimitiveRestartEnableEXT  { VK_NULL_HANDLE };
	PFN_vkCmdSetPrimitiveTopologyEXT       vkCmdSetPrimitiveTopologyEXT       { VK_NULL_HANDLE };
	PFN_vkCmdSetRasterizationSamplesEXT    vkCmdSetRasterizationSamplesEXT    { VK_NULL_HANDLE };
	PFN_vkCmdSetRasterizerDiscardEnableEXT vkCmdSetRasterizerDiscardEnableEXT { VK_NULL_HANDLE };
	PFN_vkCmdSetSampleMaskEXT              vkCmdSetSampleMaskEXT              { VK_NULL_HANDLE };
	PFN_vkCmdSetScissorWithCountEXT        vkCmdSetScissorWithCountEXT        { VK_NULL_HANDLE };
	PFN_vkCmdSetStencilTestEnableEXT       vkCmdSetStencilTestEnableEXT       { VK_NULL_HANDLE };
	PFN_vkCmdSetViewportWithCountEXT       vkCmdSetViewportWithCountEXT       { VK_NULL_HANDLE };

		// VK_EXT_vertex_input_dynamic_state
	PFN_vkCmdSetVertexInputEXT             vkCmdSetVertexInputEXT { VK_NULL_HANDLE };


public:
	ShaderObjectWindow ( int w, int h, const std::string& t, DevicePolicy * p ) : VulkanWindow ( w, h, t, true, p )
	{
		loadExtensions ();
		setController  ( new RotateController ( this, glm::vec3(2.0f, 2.0f, 2.0f) ) );

		mesh = std::unique_ptr<Mesh> ( loadMesh ( device, "../../Models/teapot.3ds", 0.04f ) );

		sampler.create  ( device );		// use default options
		texture.load    ( device, "../../Textures/Fieldstone.dds", false );

		descSetLayout
			.add    ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT )
			.add    ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )
			.create ( device.getDevice () );

		vertexShader   = createShader ( Data ( "shaders/shader-tex.vert.spv" ), VK_SHADER_STAGE_VERTEX_BIT,   &descSetLayout );
		fragmentShader = createShader ( Data ( "shaders/shader-tex.frag.spv" ), VK_SHADER_STAGE_FRAGMENT_BIT, &descSetLayout );

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
				.setLayout        ( device, descAllocator, descSetLayout )
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

		createUniformBuffers ();
		createDescriptorSets ();
		createCommandBuffers ();
	}

	virtual	void	freePipelines () override
	{
		commandBuffers.clear ();
		freeUniformBuffers   ();
		descriptorSets.clear ();
		descAllocator.clean  ();
	}
	
	virtual	void	submit ( uint32_t imageIndex ) override 
	{
		updateUniformBuffer ( imageIndex );
		defaultSubmit       ( commandBuffers [imageIndex] );
	}

	void	createCommandBuffers ()
	{
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

			auto	cmd = commandBuffers [i].getHandle ();

			pipelineBarrier ( commandBuffers [i], { barrierImage, barrierDepth } );

			vkCmdBeginRenderingKHR ( cmd, &renderingInfo );

			////////////
			VkViewport viewport = { 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
			VkRect2D   scissor  = { 0, 0, width, height };

			// No more pipelines required, everything is bound at command buffer level
			// This also means that we need to explicitly set a lot of the state to be spec compliant

			vkCmdSetViewportWithCountEXT       ( cmd, 1, &viewport );
			vkCmdSetScissorWithCountEXT        ( cmd, 1, &scissor );
			vkCmdSetCullModeEXT                ( cmd, VK_CULL_MODE_BACK_BIT );
			vkCmdSetFrontFaceEXT               ( cmd, VK_FRONT_FACE_CLOCKWISE );
			vkCmdSetDepthTestEnableEXT         ( cmd, VK_TRUE );
			vkCmdSetDepthWriteEnableEXT        ( cmd, VK_TRUE );
			vkCmdSetDepthCompareOpEXT          ( cmd, VK_COMPARE_OP_LESS );
			vkCmdSetPrimitiveTopologyEXT       ( cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST );
			vkCmdSetRasterizerDiscardEnableEXT ( cmd, VK_FALSE );
			vkCmdSetPolygonModeEXT             ( cmd, VK_POLYGON_MODE_FILL );
			vkCmdSetRasterizationSamplesEXT    ( cmd, VK_SAMPLE_COUNT_1_BIT );
			vkCmdSetAlphaToCoverageEnableEXT   ( cmd, VK_FALSE );
			vkCmdSetDepthBiasEnableEXT         ( cmd, VK_FALSE );
			vkCmdSetStencilTestEnableEXT       ( cmd, VK_FALSE );
			vkCmdSetPrimitiveRestartEnableEXT  ( cmd, VK_FALSE );

			const uint32_t sampleMask = 0xFF;
			
			vkCmdSetSampleMaskEXT              ( cmd, VK_SAMPLE_COUNT_1_BIT, &sampleMask );

			const VkBool32					colorBlendEnables        = VK_FALSE;
			const VkColorComponentFlags		colorBlendComponentFlags = 0xf;
			const VkColorBlendEquationEXT	colorBlendEquation {};

			vkCmdSetColorBlendEnableEXT       ( cmd, 0, 1, &colorBlendEnables );
			vkCmdSetColorWriteMaskEXT         ( cmd, 0, 1, &colorBlendComponentFlags );

			VkVertexInputBindingDescription2EXT vertexInputBinding {};

			vertexInputBinding.sType     = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
			vertexInputBinding.binding   = 0;
			vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexInputBinding.stride    = sizeof(BasicVertex);
			vertexInputBinding.divisor   = 1;

			std::vector<VkVertexInputAttributeDescription2EXT> vertexAttributes = 
			{
				{ VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(BasicVertex, pos) },
				{ VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 1, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(BasicVertex, tex) }
			};

			vkCmdSetVertexInputEXT  ( cmd, 1, &vertexInputBinding, uint32_t ( vertexAttributes.size () ), vertexAttributes.data () );

				// Create pipeline layout and bind it to command buffer
			VkPipelineLayoutCreateInfo			pipelineLayoutInfo = {};
			std::vector<VkDescriptorSetLayout>	layouts;
			VkDescriptorSet						descSet = descriptorSets [i].getHandle ();

			layouts.push_back ( descSetLayout.getHandle () );

			pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 0;
			pipelineLayoutInfo.setLayoutCount = (uint32_t) layouts.size ();
			pipelineLayoutInfo.pSetLayouts    = layouts.data ();

			vkCreatePipelineLayout  ( device.getDevice (), &pipelineLayoutInfo, nullptr, &pipelineLayout );
			vkCmdBindDescriptorSets ( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 0, nullptr );

			VkShaderStageFlagBits stages  [2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
			VkShaderEXT			  shaders [2] = { vertexShader, fragmentShader };

			vkCmdBindShadersEXT ( cmd, 2, stages, shaders );

				// render mesh
			commandBuffers [i].render ( mesh.get () );

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

	VkShaderEXT	createShader ( Data& data, VkShaderStageFlagBits stage, DescSetLayout * layout = nullptr, const std::string& name = "main" )
	{
		VkShaderCreateInfoEXT	shaderCreateInfo {};
		VkDescriptorSetLayout	descSetLayout = layout ? layout -> getHandle () : VK_NULL_HANDLE;
		VkShaderEXT				shader        = VK_NULL_HANDLE;

		shaderCreateInfo.sType     = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
		shaderCreateInfo.flags     = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
		shaderCreateInfo.stage     = stage;
		//shaderCreateInfo.nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderCreateInfo.codeType  = VK_SHADER_CODE_TYPE_SPIRV_EXT;
		shaderCreateInfo.pCode     = reinterpret_cast<const uint32_t*>( data.getPtr () );
		shaderCreateInfo.codeSize  = data.getLength ();
		shaderCreateInfo.pName     = name.c_str ();

		if ( layout != nullptr )
		{
			shaderCreateInfo.setLayoutCount = 1;
			shaderCreateInfo.pSetLayouts    = &descSetLayout;
		}

		if ( vkCreateShadersEXT ( device.getDevice (), 1, &shaderCreateInfo, nullptr, &shader) != VK_SUCCESS )
			warning () << "\nError creating shader object " << data.getFileName () << std::endl;

		return shader;
	}

	void	loadExtensions ()
	{
		vkCreateShadersEXT       = reinterpret_cast<PFN_vkCreateShadersEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCreateShadersEXT"));
		vkDestroyShaderEXT       = reinterpret_cast<PFN_vkDestroyShaderEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkDestroyShaderEXT"));
		vkCmdBindShadersEXT      = reinterpret_cast<PFN_vkCmdBindShadersEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdBindShadersEXT"));
		vkGetShaderBinaryDataEXT = reinterpret_cast<PFN_vkGetShaderBinaryDataEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkGetShaderBinaryDataEXT"));

		vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdBeginRenderingKHR"));
		vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdEndRenderingKHR"));

		vkCmdSetAlphaToCoverageEnableEXT   = reinterpret_cast<PFN_vkCmdSetAlphaToCoverageEnableEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetAlphaToCoverageEnableEXT"));
		vkCmdSetColorBlendEnableEXT        = reinterpret_cast<PFN_vkCmdSetColorBlendEnableEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetColorBlendEnableEXT"));
		vkCmdSetColorWriteMaskEXT          = reinterpret_cast<PFN_vkCmdSetColorWriteMaskEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetColorWriteMaskEXT"));
		vkCmdSetCullModeEXT                = reinterpret_cast<PFN_vkCmdSetCullModeEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetCullModeEXT"));
		vkCmdSetDepthBiasEnableEXT         = reinterpret_cast<PFN_vkCmdSetDepthBiasEnableEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetDepthBiasEnableEXT"));
		vkCmdSetDepthCompareOpEXT          = reinterpret_cast<PFN_vkCmdSetDepthCompareOpEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetDepthCompareOpEXT"));
		vkCmdSetDepthTestEnableEXT         = reinterpret_cast<PFN_vkCmdSetDepthTestEnableEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetDepthTestEnableEXT"));
		vkCmdSetDepthWriteEnableEXT        = reinterpret_cast<PFN_vkCmdSetDepthWriteEnableEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetDepthWriteEnableEXT"));
		vkCmdSetFrontFaceEXT               = reinterpret_cast<PFN_vkCmdSetFrontFaceEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetFrontFaceEXT"));
		vkCmdSetPolygonModeEXT             = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetPolygonModeEXT"));
		vkCmdSetPrimitiveRestartEnableEXT  = reinterpret_cast<PFN_vkCmdSetPrimitiveRestartEnableEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetPrimitiveRestartEnableEXT"));
		vkCmdSetPrimitiveTopologyEXT       = reinterpret_cast<PFN_vkCmdSetPrimitiveTopologyEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetPrimitiveTopologyEXT"));
		vkCmdSetRasterizationSamplesEXT    = reinterpret_cast<PFN_vkCmdSetRasterizationSamplesEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetRasterizationSamplesEXT"));
		vkCmdSetRasterizerDiscardEnableEXT = reinterpret_cast<PFN_vkCmdSetRasterizerDiscardEnableEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetRasterizerDiscardEnableEXT"));
		vkCmdSetSampleMaskEXT              = reinterpret_cast<PFN_vkCmdSetSampleMaskEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetSampleMaskEXT"));
		vkCmdSetScissorWithCountEXT        = reinterpret_cast<PFN_vkCmdSetScissorWithCountEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetScissorWithCountEXT"));
		vkCmdSetStencilTestEnableEXT       = reinterpret_cast<PFN_vkCmdSetStencilTestEnableEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetStencilTestEnableEXT"));
		vkCmdSetVertexInputEXT             = reinterpret_cast<PFN_vkCmdSetVertexInputEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetVertexInputEXT"));
		vkCmdSetViewportWithCountEXT       = reinterpret_cast<PFN_vkCmdSetViewportWithCountEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetViewportWithCountEXT"));;
	}
};

int main ( int argc, const char * argv [] ) 
{
	DevicePolicy	policy;
	VkPhysicalDeviceShaderObjectFeaturesEXT enabledShaderObjectFeaturesEXT {};

	enabledShaderObjectFeaturesEXT.sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
	enabledShaderObjectFeaturesEXT.shaderObject = VK_TRUE;

	policy.addDeviceExtension ( VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME                );
	policy.addDeviceExtension ( VK_EXT_SHADER_OBJECT_EXTENSION_NAME                    );
	policy.addDeviceExtension ( VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME       );
	policy.addDeviceExtension ( VK_KHR_MAINTENANCE2_EXTENSION_NAME                     );
	policy.addDeviceExtension ( VK_KHR_MULTIVIEW_EXTENSION_NAME                        );
	policy.addDeviceExtension ( VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME              );
	policy.addDeviceExtension ( VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME            );
	//policy.addDeviceExtension ( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );

	policy.addFeatures ( &enabledShaderObjectFeaturesEXT );

	return ShaderObjectWindow ( 800, 600, "Shader objects with dynamic rendering", &policy ).run ();
}
