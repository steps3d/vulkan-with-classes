#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"stb_image.h"

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

	PFN_vkTransitionImageLayoutEXT				vkTransitionImageLayoutEXT              = {};
	PFN_vkCopyMemoryToImageEXT					vkCopyMemoryToImageEXT                  = {};
	PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2 = {};

public:
	ExampleWindow ( int w, int h, const std::string& t, bool depth, DevicePolicy * p ) : VulkanWindow ( w, h, t, depth, p )
	{
		loadExtensions ();

		vertexBuffer.create  ( device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, Buffer::hostWrite );
		indexBuffer.create   ( device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  indices,  Buffer::hostWrite );
		sampler.setMinFilter ( VK_FILTER_LINEAR ).setMagFilter ( VK_FILTER_LINEAR ).create ( device );

		createTexture   ();
		createPipelines ();
	}

	void	loadExtensions ()
	{
		vkTransitionImageLayoutEXT           = reinterpret_cast<PFN_vkTransitionImageLayoutEXT>  (vkGetDeviceProcAddr(device.getDevice (), "vkTransitionImageLayoutEXT"));
		vkCopyMemoryToImageEXT               = reinterpret_cast<PFN_vkCopyMemoryToImageEXT>      (vkGetDeviceProcAddr(device.getDevice (), "vkCopyMemoryToImageEXT"));
		vkGetPhysicalDeviceFormatProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties2> (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties2"));
	}

		// Check if the image format supports the host image copy flag
		// Note: All formats that support sampling are required to support this flag
	bool	formatSupportHostCopy ( VkFormat format )
	{
		VkFormatProperties2KHR formatProperties2 = {};
		VkFormatProperties3KHR formatProperties3 = {};

		formatProperties3.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3_KHR;
		formatProperties2.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR;
		formatProperties2.pNext = &formatProperties3;	// Properties3 need to be chained into Properties2

		vkGetPhysicalDeviceFormatProperties2 ( device.getPhysicalDevice (), format, &formatProperties2 );

		return (formatProperties3.optimalTilingFeatures & VK_FORMAT_FEATURE_2_HOST_IMAGE_TRANSFER_BIT_EXT) != 0;
	}

	void createTexture ()
	{
		int				texWidth, texHeight, texChannels;
		stbi_uc       * pixels    = stbi_load ( "textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
		VkDeviceSize	imageSize = texWidth * texHeight * 4;
		uint32_t		mipLevels = 1;	//static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
		VkFormat		format    = VK_FORMAT_R8G8B8A8_UNORM;

		if ( !pixels )
			fatal () << "failed to load texture image!";

		if ( !formatSupportHostCopy ( format ) )
			fatal () << "Host image copy not supported for format" << std::endl;

			// TRANSFER_SRC for mipmap calculations via vkCmdBlitImage
		texture.create ( device, texWidth, texHeight, 1, mipLevels, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT, 0 /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/);

		std::vector<VkMemoryToImageCopyEXT> copies;
		VkImageSubresourceRange				subresourceRange = {};

		for ( uint32_t i = 0; i < mipLevels; i++ )
		{
				// Setup a buffer image copy structure for the current mip level
			VkMemoryToImageCopyEXT memoryCopy = {};

			memoryCopy.sType                           = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT;
			memoryCopy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			memoryCopy.imageSubresource.mipLevel       = i;
			memoryCopy.imageSubresource.baseArrayLayer = 0;
			memoryCopy.imageSubresource.layerCount     = 1;
			memoryCopy.imageExtent.width               = texWidth  >> i;
			memoryCopy.imageExtent.height              = texHeight >> i;
			memoryCopy.imageExtent.depth               = 1;

				// This tells the implementation where to read the data from
			//ktx_size_t     offset;
			//KTX_error_code ret = ktxTexture_GetImageOffset(ktx_texture, i, 0, 0, &offset);
			//assert(ret == KTX_SUCCESS);
			memoryCopy.pHostPointer = pixels + 4 * i * memoryCopy.imageExtent.width * memoryCopy.imageExtent.height;

			copies.push_back ( memoryCopy );
		}

		subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount   = mipLevels;
		subresourceRange.layerCount   = 1;

		// VK_EXT_host_image_copy also introduces a simplified way of doing the required image transition on the host
		// This no longer requires a dedicated command buffer to submit the barrier
		// We also no longer need multiple transitions, and only have to do one for the final layout
		VkHostImageLayoutTransitionInfoEXT host_image_layout_transition_info = {};

		host_image_layout_transition_info.sType            = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT;
		host_image_layout_transition_info.image            = texture.getImage().getHandle ();
		host_image_layout_transition_info.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
		host_image_layout_transition_info.newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		host_image_layout_transition_info.subresourceRange = subresourceRange;

		vkTransitionImageLayoutEXT ( device.getDevice (), 1, &host_image_layout_transition_info );

		// With the image in the correct layout and copy information for all mip levels setup, 
		// we can now issue the copy to our taget image from the host
		// The implementation will then convert this to an implementation specific optimal tiling layout
		VkCopyMemoryToImageInfoEXT copyInfo = {};

		copyInfo.sType          = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT;
		copyInfo.dstImage       = texture.getImage ().getHandle ();
		copyInfo.dstImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		copyInfo.regionCount    = uint32_t ( copies.size () );
		copyInfo.pRegions       = copies.data ();

		vkCopyMemoryToImageEXT ( device.getDevice (), &copyInfo );

		stbi_image_free ( pixels );
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
				.pipeline          ( pipeline )
				.bindVertexBuffers ( { {vertexBuffer, 0} } )
				.bindIndexBuffer   ( indexBuffer )
				.addDescriptorSets ( { descriptorSets [i] } )
				.setViewport       ( swapChain.getExtent () )
				.setScissor        ( swapChain.getExtent () )
				.drawIndexed       ( static_cast<uint32_t>(indices.size ()) )
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
	DevicePolicy	policy;

		// we require VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME device extension
	policy.addDeviceExtension ( VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME  );

	VkPhysicalDeviceHostImageCopyFeaturesEXT	hostCopyFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES_EXT };
	hostCopyFeatures.hostImageCopy = VK_TRUE;

	policy.addFeatures   ( &hostCopyFeatures );

	return ExampleWindow ( 800, 600, "Test window", true, &policy ).run ();
}
