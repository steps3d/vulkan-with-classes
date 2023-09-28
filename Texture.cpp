#include	"Device.h"
#include	"Buffer.h"
#include	"Texture.h"
#include	"Data.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include	"stb_image.h"
#include	"stb_image_write.h"

bool	Image::create ( Device& dev, uint32_t w, uint32_t h, uint32_t d, uint32_t numMipLevels, VkFormat fmt, VkImageTiling tl, 
				 VkImageUsageFlags usage, int mappable, VkImageLayout initialLayout )
{
	VkImageCreateInfo imageInfo = {};

	device                  = &dev;
	width                   = w;
	height                  = h;
	depth                   = d;
	format                  = fmt;
	tiling                  = tl;
	mipLevels               = numMipLevels;
	device                  = &dev;
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width  = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = mipLevels;
	imageInfo.arrayLayers   = 1;
	imageInfo.format        = format;
	imageInfo.tiling        = tiling;
	imageInfo.initialLayout = initialLayout;
	imageInfo.usage         = usage;
	imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

#ifdef USE_VMA
	VmaAllocationCreateInfo	allocInfo = {};

	allocInfo.usage    = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags    = 0;
	allocInfo.priority = 1.0f;

	if ( mappable & hostRead )
		allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	if ( mappable & hostWrite )
		allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	if ( vmaCreateImage ( device->getAllocator (), &imageInfo, &allocInfo, &image, &allocation, nullptr ) != VK_SUCCESS ) 
		fatal () << "vmaImage: Cannot create image" << std::endl;
#else
	if ( vkCreateImage ( dev.getDevice (), &imageInfo, nullptr, &image ) != VK_SUCCESS ) 
		fatal () << "Image: Cannot create image";

	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ;

	if ( mappable )
		properties |=  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VkMemoryRequirements memRequirements;

	vkGetImageMemoryRequirements ( dev.getDevice (), image, &memRequirements );

	if ( !memory.alloc ( dev, memRequirements, properties ) )
		fatal () << "Image: Cannot alloc memory for image";

	vkBindImageMemory ( dev.getDevice (), image, memory.getMemory (), 0 );
#endif // USE_VMA

	return true;
}

bool	Image::create ( Device& dev, ImageCreateInfo& info, int mappable )
{
	width                   = info.data ()->extent.width;
	height                  = info.data ()->extent.height;
	depth                   = info.data ()->extent.depth;
	format                  = info.data ()->format;
	tiling                  = info.data ()->tiling;
	mipLevels               = info.data ()->mipLevels;
	arrayLayers             = info.data ()->arrayLayers;
	device                  = &dev;

#ifdef USE_VMA
	VmaAllocationCreateInfo	allocInfo = {};

	device             = &dev;
	allocInfo.usage    = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags    = 0;
	allocInfo.priority = 1.0f;

	if ( mappable & hostRead )
		allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	if ( mappable & hostWrite )
		allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	if ( vmaCreateImage ( device->getAllocator (), info.data (), &allocInfo, &image, &allocation, nullptr ) != VK_SUCCESS ) 
		fatal () << "vmaImage: Cannot create image" << std::endl;
#else
	if ( vkCreateImage ( dev.getDevice (), info.data (), nullptr, &image ) != VK_SUCCESS ) 
		fatal () << "Image: Cannot create image";

	VkMemoryRequirements	memRequirements;
	VkMemoryPropertyFlags	properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ;

	if ( mappable )
		properties |=  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	vkGetImageMemoryRequirements ( dev.getDevice (), image, &memRequirements );

	if ( !memory.alloc ( dev, memRequirements, properties ) )
		fatal () << "Image: Cannot alloc memory for image";

	vkBindImageMemory ( dev.getDevice (), image, memory.getMemory (), 0 );
#endif // USE_VMA

	return true;
}

void	Image :: transitionLayout ( SingleTimeCommand& cmd, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout )
{
	VkImageMemoryBarrier	barrier       = {};
	VkPipelineStageFlags 	sourceStage;
	VkPipelineStageFlags 	destinationStage;

	barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout                       = oldLayout;
	barrier.newLayout                       = newLayout;
	barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                           = image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = arrayLayers;

	if ( newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if ( format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT )
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	} 
	else
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		
	if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL )
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else
	if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} 
	else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} 
	else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage      = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
		fatal () << "Texture: Unsupported layout transition!";

	vkCmdPipelineBarrier (
		cmd.getHandle (),
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier );
}

void	Image::copyFromBuffer ( SingleTimeCommand& cmd, Buffer& buffer, uint32_t width, uint32_t height, uint32_t depth, uint32_t layers, uint32_t mipLevel )
{
	VkBufferImageCopy	region        = {};
		
	region.bufferOffset                    = 0;
	region.bufferRowLength                 = 0;
	region.bufferImageHeight               = 0;
	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel       = mipLevel;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount     = layers;
	region.imageOffset                     = {0, 0, 0};
	region.imageExtent                     = { width, height, depth };

	vkCmdCopyBufferToImage ( cmd.getHandle (), buffer.getHandle (), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
}

uint32_t	Image::calcNumMipLevels ( uint32_t w, uint32_t h, uint32_t d )
{
	auto	size = std::max ( std::max ( w, h ), d );
		
	return static_cast<uint32_t>( std::floor ( std::log2 ( size ) ) ) + 1;
}
	
void	Image::transitionLayout ( SingleTimeCommand& cmd, VkImage image, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage, 
				   VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout )
{
	VkImageMemoryBarrier	barrier       = {};

	barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout                       = oldLayout;
	barrier.newLayout                       = newLayout;
	barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                           = image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1;	// XXX
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;	// XXX
	barrier.srcAccessMask                   = srcAccessMask;
	barrier.dstAccessMask                   = dstAccessMask;

	vkCmdPipelineBarrier (
		cmd.getHandle (),
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier );
}

void	Image::saveAs ( const std::string& fileName )
{
/*
	{
		SingleTimeCommand	cmd ( getDevice () );

		transitionLayout  ( cmd, getFormat (), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL );
	}

	VkImageSubresource	subResource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout	subResourceLayout;

	vkGetImageSubresourceLayout ( getDevice ().getDevice (), getHandle (), &subResource, &subResourceLayout );

	const uint8_t * data = (const uint8_t *)getMemory ().map ();

				// what # of components
	stbi_write_png ( fileName.c_str(), getWidth (), getHeight (), 4, data, 4*getWidth () );

	image.getMemory().unmap ();

			// return back to optimal layout ???
	{
		SingleTimeCommand	cmd ( getDevice () );

		image.transitionLayout ( cmd, image.getFormat (), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	}
*/
}

void Sampler::create ( Device& dev ) 
{
	device = dev.getDevice ();
		
	VkSamplerCreateInfo samplerInfo = {};
	
	samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter               = magFilter;
	samplerInfo.minFilter               = minFilter;
	samplerInfo.addressModeU            = addressModeU;
	samplerInfo.addressModeV            = addressModeV;
	samplerInfo.addressModeW            = addressModeW;
	samplerInfo.anisotropyEnable        = anisotropy;
	samplerInfo.maxAnisotropy           = maxAnisotropy;
	samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = unnormalized;
	samplerInfo.compareEnable           = VK_FALSE;
	samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode              = mipmapMode;
	samplerInfo.mipLodBias              = lodBias;
	samplerInfo.minLod                  = minLod;
	samplerInfo.maxLod                  = maxLod;

	if ( vkCreateSampler ( device, &samplerInfo, nullptr, &sampler ) != VK_SUCCESS )
		fatal () << "Sampler: failed to create texture sampler!";
}

void	Texture::createImageView ( VkImageAspectFlags aspectFlags, VkImageViewType viewType )
{
	assert ( image.getHandle () != VK_NULL_HANDLE );
		
	VkImageViewCreateInfo viewInfo = {};
		
	viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                           = image.getHandle ();
	viewInfo.viewType                        = viewType;
	viewInfo.format                          = image.getFormat ();
	viewInfo.subresourceRange.aspectMask     = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = image.getMipLevels ();
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount     = 1;

	if ( vkCreateImageView ( image.getDevice ()->getDevice (), &viewInfo, nullptr, &imageView ) != VK_SUCCESS )
		fatal () << "Texture: failed to create texture image view!";
}

Texture&	Texture::create ( Device& dev, uint32_t w, uint32_t h, uint32_t d, uint32_t mipLevels, VkFormat fmt, VkImageTiling tl, 
					 VkImageUsageFlags usage, int mapping, VkImageLayout initialLayout )
{
	image.create    ( dev, w, h, d, mipLevels, fmt, tl, usage, mapping );
		
			// check for depth image
	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		
	if ( fmt == VK_FORMAT_D32_SFLOAT || fmt == VK_FORMAT_D32_SFLOAT_S8_UINT || fmt == VK_FORMAT_D24_UNORM_S8_UINT )
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
		
	createImageView ( aspectFlags );
		
	return *this;
}

void	Texture::generateMipmaps ( SingleTimeCommand& cmd, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels ) 
{
			// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
		
	vkGetPhysicalDeviceFormatProperties ( image.getPhysicalDevice (), imageFormat, &formatProperties );

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT ) )
		fatal () << "Texture: texture image format does not support linear blitting!";

	VkImageMemoryBarrier	barrier   = {};
	int32_t					mipWidth  = image.getWidth  ();
	int32_t					mipHeight = image.getHeight ();
		
	barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image                           = image.getHandle ();
	barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;
	barrier.subresourceRange.levelCount     = 1;

	for ( uint32_t i = 1; i < mipLevels; i++ ) 
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier ( cmd.getHandle (),
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier );

		VkImageBlit blit = {};
			
		blit.srcOffsets[0]                 = {0, 0, 0};
		blit.srcOffsets[1]                 = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel       = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount     = 1;
		blit.dstOffsets[0]                 = {0, 0, 0};
		blit.dstOffsets[1]                 = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel       = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount     = 1;

		vkCmdBlitImage ( cmd.getHandle (),
			image.getHandle (), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image.getHandle (), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit, VK_FILTER_LINEAR );

		barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier ( cmd.getHandle (),
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier );

		if ( mipWidth > 1 )
			mipWidth /= 2;
				
		if ( mipHeight > 1 )
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier ( cmd.getHandle (),
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr, 0, nullptr, 1, &barrier );
}

	// add .hdr and .dds support !!!
bool Texture::load ( Device& dev, const std::string& fileName, bool mipmaps, bool srgb )
{
	Data			data ( fileName );

	if ( !data.isOk () )
	{
		log () << "Texture: error opening " << fileName << Log::endl;

		return false;
	}

	if ( loadDds ( dev, *this, data ) )		// check for .dds texture
		return true;

	int				texWidth, texHeight, texChannels;
	int				unitSize  = 1;		// 1 byte per component
	VkFormat		fmt;
	VkDeviceSize	imageSize;
	Buffer			stagingBuffer;

		// check for .hdr file
	float * vals = stbi_loadf_from_memory ( (const stbi_uc *)data.getPtr (), data.getLength (), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );

	if ( vals )
	{
		unitSize  = sizeof ( float );
		fmt       = VK_FORMAT_R32G32B32A32_SFLOAT;			// only one RGBA 32F format, no SRGB
		imageSize = texWidth * texHeight * 4 * unitSize;

		stagingBuffer.create ( dev, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, Buffer::hostWrite );	//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		stagingBuffer.copy   ( vals, imageSize );

		stbi_image_free ( vals );
	}
	else
	{
		stbi_uc * pixels = stbi_load_from_memory ( (const stbi_uc *)data.getPtr ( 0 ), data.getLength (), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );

		if ( !pixels )
		{
			log () << "Texture: failed to load texture image! " << fileName << Log::endl;
			return false;
		}

		imageSize = texWidth * texHeight * 4;
		fmt       = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

		stagingBuffer.create ( dev, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, Buffer::hostWrite );	//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		stagingBuffer.copy   ( pixels, imageSize );

		stbi_image_free ( pixels );
	}

	uint32_t		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		// TRANSFER_SRC for mipmap calculations via vkCmdBlitImage
	create ( dev, texWidth, texHeight, 1, mipLevels, fmt, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0 );

	{
		SingleTimeCommand	cmd ( dev );
			
		image.transitionLayout  ( cmd, image.getFormat (), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		image.copyFromBuffer    ( cmd, stagingBuffer, texWidth, texHeight, 1 );
			
		if ( mipmaps )
			generateMipmaps     ( cmd, image.getFormat (), texWidth, texHeight, mipLevels );
		else
			image.transitionLayout ( cmd, image.getFormat (), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	}

	return true;
}

bool	Texture :: loadRaw ( Device& dev, int texWidth, int texHeight, const void * pixels, VkFormat format, bool mipmaps )
{
	int				bpp       = 4;

	if ( format == VK_FORMAT_R8_UNORM || format == VK_FORMAT_R8_SNORM || format == VK_FORMAT_R8_USCALED || format == VK_FORMAT_R8_SSCALED || format == VK_FORMAT_R8_USCALED || format == VK_FORMAT_R8_SSCALED ||
		 format == VK_FORMAT_R8_UINT  || format == VK_FORMAT_R8_SINT  || format == VK_FORMAT_R8_SRGB    || format == VK_FORMAT_R8_UINT    || format == VK_FORMAT_R8_SINT    || format == VK_FORMAT_R8_SRGB )
		bpp = 1;
	else
	if ( format == VK_FORMAT_R8G8_UNORM || format == VK_FORMAT_R8G8_SNORM || format == VK_FORMAT_R8G8_USCALED || format == VK_FORMAT_R8G8_SSCALED || 
		 format == VK_FORMAT_R8G8_UINT  || format == VK_FORMAT_R8G8_SINT  || format == VK_FORMAT_R8G8_SRGB )
		bpp = 2;

	VkDeviceSize	imageSize = texWidth * texHeight * bpp;
	uint32_t		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	Buffer			stagingBuffer;

	stagingBuffer.create ( dev, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	stagingBuffer.copy   ( pixels, imageSize );

		// TRANSFER_SRC for mipmap calculations via vkCmdBlitImage
	create ( dev, texWidth, texHeight, 1, mipLevels, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0 );

	{
		SingleTimeCommand	cmd ( dev );

		image.transitionLayout  ( cmd, image.getFormat (), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		image.copyFromBuffer    ( cmd, stagingBuffer, texWidth, texHeight, 1 );

		if ( mipmaps )
			generateMipmaps     ( cmd, image.getFormat (), texWidth, texHeight, mipLevels );
		else
			image.transitionLayout ( cmd, image.getFormat (), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	}

	return true;
}

bool	Texture::loadCubemap ( Device& dev, const std::vector<const char *>& files, bool mipmaps, bool srgb )
{
	if ( files.size () != 6 )
		return false;

	struct
	{
		int			width, height, numChannels;
		stbi_uc   * pixels;
	} faces [6];
		
	for ( int i = 0; i < 6; i++ )
	{
		faces [i].pixels = stbi_load ( files [i], &faces [i].width, &faces [i].height, &faces [i].numChannels, STBI_rgb_alpha );
			
		if ( faces [i].width != faces [i].height )
			return false;

		if ( faces [i].pixels == nullptr )
			return false;
	}
		
	assert ( faces [0].width == faces [1].width && faces [2].width == faces [3].width && faces [4].width == faces [5].width && faces [1].width == faces [2].width && faces [3].width == faces [4].width );
		
	auto			width     = faces [0].width;
	VkDeviceSize	faceSize  = width * width * 4;
	VkDeviceSize	imageSize = faceSize * 6;
	uint32_t		mipLevels = static_cast<uint32_t>(std::floor(std::log2(width))) + 1;
	VkFormat		fmt       = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	Buffer			stagingBuffer;
		
	if ( !mipmaps )
		mipLevels = 1;
		
			// copy all data to staging buffer
	stagingBuffer.create ( dev, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		
	for ( int i = 0; i < 6; i++ )
	{
		stagingBuffer.copy   ( faces [i].pixels, faceSize, i*faceSize );

		stbi_image_free ( faces [i].pixels );
	}
		
			// create layered 2D image with 6 layers
	image.create ( dev, ImageCreateInfo ( width, width ).setFlags ( VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ).setFormat ( fmt ).setMipLevels ( mipLevels ).setLayers ( 6 ).setUsage ( VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT ), 0 );
		
			// check for depth image
	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		
	if ( image.getFormat () == VK_FORMAT_D32_SFLOAT || image.getFormat () == VK_FORMAT_D32_SFLOAT_S8_UINT || image.getFormat () == VK_FORMAT_D24_UNORM_S8_UINT )
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
		
	{
		SingleTimeCommand	cmd ( dev );
			
		image.transitionLayout  ( cmd, image.getFormat (), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		image.copyFromBuffer    ( cmd, stagingBuffer, width, width, 1, 6 );

		if ( mipmaps )
			generateMipmaps     ( cmd, image.getFormat (), width, width, mipLevels );
		else
			image.transitionLayout ( cmd, image.getFormat (), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	}
		
	createImageView ( aspectFlags, VK_IMAGE_VIEW_TYPE_CUBE );

	return true;
}

bool	isDepthFormat ( VkFormat format )
{
	const std::vector<VkFormat> formats = 
	{
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
	};

	return std::find ( formats.begin(), formats.end(), format ) != std::end(formats);
}

bool isStencilFormat ( VkFormat format )
{
	const std::vector<VkFormat> formats = 
	{
		VK_FORMAT_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
	};

	return std::find ( formats.begin(), formats.end(), format ) != std::end(formats);
}
