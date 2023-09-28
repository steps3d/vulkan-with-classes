
#pragma once

#include	"Buffer.h"
#include	"SingleTimeCommand.h"

#include	<algorithm>			// for std::max
#include	<cmath>				// for log2

#undef	min
#undef	max

bool	isDepthFormat   ( VkFormat format );
bool	isStencilFormat ( VkFormat );

class	ImageCreateInfo
{
	VkImageCreateInfo imageInfo = {};
	
public:
	ImageCreateInfo ( uint32_t width, uint32_t height, uint32_t depth = 1 )
	{
		imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.extent.width  = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth  = depth;
		imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.imageType     = VK_IMAGE_TYPE_3D;
		imageInfo.arrayLayers   = 1;
		imageInfo.mipLevels     = 1;
		
		if ( height == 1 && depth == 1 )
			imageInfo.imageType = VK_IMAGE_TYPE_1D;
		else
		if ( depth == 1 )
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
	}
	
	ImageCreateInfo&	setFormat ( VkFormat format )
	{
		imageInfo.format = format;
		return *this;
	}

	ImageCreateInfo&	setTiling ( VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL )
	{
		imageInfo.tiling = tiling;
		return *this;
	}
	
	ImageCreateInfo&	setMipLevels ( uint32_t mipLevels )
	{
		imageInfo.mipLevels = mipLevels;
		return *this;
	}
		
	ImageCreateInfo&	setLayers ( uint32_t layers )
	{
		imageInfo.arrayLayers = layers;
		return *this;
	}
	
	ImageCreateInfo&	setInitialLayout ( VkImageLayout layout )
	{
		imageInfo.initialLayout = layout;
		return *this;
	}

	ImageCreateInfo&	setUsage ( VkImageUsageFlags flags )
	{
		imageInfo.usage = flags;
		return *this;
	}
	
	ImageCreateInfo&	setFlags ( VkImageCreateFlags flags )
	{
		imageInfo.flags = flags;
		return *this;
	}

	const VkImageCreateInfo * data () const
	{
		return &imageInfo;
	}
};

class Image 
{
	int					width = 0, height = 0, depth = 0;
	VkImage				image       = VK_NULL_HANDLE;
	VkImageType			type        = VK_IMAGE_TYPE_2D;
	uint32_t			mipLevels   = 1;
	uint32_t			arrayLayers = 1;
	VkImageTiling		tiling      = VK_IMAGE_TILING_OPTIMAL;
	VkFormat			format      = VK_FORMAT_R8G8B8A8_UNORM;
	Device			  * device      = nullptr;
#ifdef USE_VMA
	VmaAllocation		allocation  = VK_NULL_HANDLE;
#else
	GpuMemory			memory;
#endif // USE_VMA

public:
	enum
	{
		hostRead  = 1, 
		hostWrite = 2
	};

	Image  () = default;
#ifdef USE_VMA
	Image  ( Image&& im )
	{
		std::swap ( allocation,  im.allocation  );
		std::swap ( width,       im.width       );
		std::swap ( height,      im.height      );
		std::swap ( depth,       im.depth       );
		std::swap ( image,       im.image       );
		std::swap ( type,        im.type        );
		std::swap ( mipLevels,   im.mipLevels   );
		std::swap ( arrayLayers, im.arrayLayers );
		std::swap ( tiling,      im.tiling      );
		std::swap ( format,      im.format      );
		std::swap ( device,      im.device      );
	}
#else
	Image  ( Image&& im ) : memory ( std::move ( im.memory ) )
	{
		std::swap ( width,       im.width  );
		std::swap ( height,      im.height );
		std::swap ( depth,       im.depth  );
		std::swap ( image,       im.image  );
		std::swap ( type,        im.type   );
		std::swap ( mipLevels,   im.mipLevels   );
		std::swap ( arrayLayers, im.arrayLayers );
		std::swap ( tiling,      im.tiling      );
		std::swap ( format,      im.format      );
		std::swap ( device,      im.device      );
	}
#endif // USE_VMA
	Image ( const Image& ) = delete;
	~Image ()
	{
		clean ();
	}

	Image& operator = ( const Image& ) = delete;

	bool	isOk () const
	{
#ifdef USE_VMA
		return allocation != VK_NULL_HANDLE && image != VK_NULL_HANDLE;
#else
		return memory.isOk () && image != VK_NULL_HANDLE;
#endif // USE_VMA
	}

	void	clean ()
	{
#ifdef USE_VMA
		if ( device && image )
			vmaDestroyImage ( device->getAllocator (), image, allocation );

		allocation = VK_NULL_HANDLE;
#else
		if ( image != VK_NULL_HANDLE )
			vkDestroyImage ( memory.getDevice (), image, nullptr );

		memory.clean ();
#endif // USE_VMA

		image = VK_NULL_HANDLE;
	}

	uint32_t	getWidth () const
	{
		return width;
	}
	
	uint32_t	getHeight () const
	{
		return height;
	}
	
	uint32_t	getDepth () const
	{
		return depth;
	}

	uint32_t	getMipLevels () const
	{
		return mipLevels;
	}
	
	VkImage	getHandle () const
	{
		return image;
	}
	
	Device * getDevice () const
	{
		return device;
	}

	VkPhysicalDevice	getPhysicalDevice () const
	{
#ifdef USE_VMA
		return device->getPhysicalDevice ();
#else
		return memory.getPhysicalDevice ();
#endif // USE_VMA
	}

	VkFormat	getFormat () const
	{
		return format;
	}
	
#ifndef USE_VMA
	GpuMemory&	getMemory ()
	{
		return memory;
	}
#endif // USE_VMA

	bool hasDepth () const
	{
		return isDepthFormat ( format );
	}

	bool hasStencil () const
	{
		return isStencilFormat ( format );
	}

	bool isDepthStencil ()
	{
		return hasDepth () || hasStencil ();
	}

	void * map ()
	{
#ifdef USE_VMA
		void * ptr;

		if ( vmaMapMemory ( device->getAllocator (), allocation, &ptr ) != VK_SUCCESS )
			fatal () << "Image::map error" << std::endl;

		return ptr;
#else
		return image.getMemory ().map ();
#endif // USE_VMA
	}

	void	unmap ()
	{
#ifdef USE_VMA
		vmaUnmapMemory ( device->getAllocator (), allocation );
#else
		image.getMemory ().map ();
#endif // USE_VMA
	}

	bool	create ( Device& dev, uint32_t w, uint32_t h, uint32_t d, uint32_t numMipLevels, VkFormat fmt, VkImageTiling tl, 
					 VkImageUsageFlags usage, int mapping, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );

	bool	create           ( Device& dev, ImageCreateInfo& info, int mapping );
	void	transitionLayout ( SingleTimeCommand& cmd, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout );
	void	copyFromBuffer   ( SingleTimeCommand& cmd, Buffer& buffer, uint32_t width, uint32_t height, uint32_t depth = 1, uint32_t layers = 1, uint32_t mipLevel = 0 );
	void	saveAs           ( const std::string& fileName );

	static uint32_t	calcNumMipLevels ( uint32_t w, uint32_t h, uint32_t d = 1 );

	static void	transitionLayout ( SingleTimeCommand& cmd, VkImage image, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destStage, 
								   VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, 
								   VkImageLayout oldLayout, VkImageLayout newLayout );
};

class	Sampler
{
	VkDevice				device        = VK_NULL_HANDLE;
	VkSampler				sampler       = VK_NULL_HANDLE;
	VkFilter				minFilter     = VK_FILTER_NEAREST;	//LINEAR;
	VkFilter				magFilter     = VK_FILTER_NEAREST;	//LINEAR;
	VkSamplerAddressMode	addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode	addressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode	addressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkBool32				anisotropy    = VK_FALSE;
	float					maxAnisotropy = 1.0f;		// 16
	VkBool32				unnormalized  = VK_FALSE;
	VkSamplerMipmapMode		mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	float					lodBias       = 0;
	float					minLod        = 0;
	float					maxLod        = 0;
	
public:
	Sampler () {}
	Sampler ( Sampler&& s )
	{
		std::swap ( device,        s.device        );
		std::swap ( sampler,       s.sampler       );
		std::swap ( minFilter,     s.minFilter     );
		std::swap ( magFilter,     s.magFilter     ); 
		std::swap ( addressModeU,  s.addressModeU  );
		std::swap ( addressModeV,  s.addressModeV  );
		std::swap ( addressModeW,  s.addressModeW  );
		std::swap ( anisotropy,    s.anisotropy    );
		std::swap ( maxAnisotropy, s.maxAnisotropy );
		std::swap ( unnormalized,  s.unnormalized  );
		std::swap ( mipmapMode,    s.mipmapMode    );
		std::swap ( lodBias,       s.lodBias       );
		std::swap ( minLod,        s.minLod        );
		std::swap ( maxLod,        s.maxLod        );
	}
	Sampler ( const Sampler& ) = delete;
	~Sampler ()
	{
		clean ();
	}
	
	Sampler& operator = ( const Sampler& ) = delete;

	bool	isOk () const
	{
		return device != nullptr && sampler != VK_NULL_HANDLE;
	}

	VkSampler	getHandle () const
	{
		return sampler;
	}
	
	void	clean ()
	{
		if ( sampler != VK_NULL_HANDLE )
	        vkDestroySampler ( device, sampler, nullptr );
		
		sampler = VK_NULL_HANDLE;
	}

	Sampler&	setMinFilter ( VkFilter filter )
	{
		minFilter = filter;
		return *this;
	}
	
	Sampler&	setMagFilter ( VkFilter filter )
	{
		magFilter = filter;
		return *this;
	}
	
	Sampler&	setAddressMode ( VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w )
	{
		addressModeU = u;
		addressModeV = v;
		addressModeW = w;
		return *this;
	}
	
	Sampler&	setAnisotropy ( bool enable, float maxAniso = 0 )
	{
		anisotropy    = enable ? VK_TRUE : VK_FALSE;
		maxAnisotropy = maxAniso;
		return *this;
	}
	
	Sampler&	setNormalized ( bool flag )
	{
		unnormalized = flag ? VK_FALSE : VK_TRUE;
		return *this;
	}
	
	Sampler&	setMipmapMode ( VkSamplerMipmapMode mode )
	{
		mipmapMode = mode;
		return *this;
	}
	
	Sampler&	setMipmapBias ( float bias )
	{
		lodBias = bias;
		return *this;
	}
	
	Sampler&	setMinLod ( float v )
	{
		minLod = v;
		return *this;
	}
	
	Sampler&	setMaxLod ( float v )
	{
		maxLod = v;
		return *this;
	}
	
	void create ( Device& dev );
};

class Texture
{
	Image			image;
	VkImageView 	imageView = VK_NULL_HANDLE;

public:
	Texture () = default;
	Texture ( Texture&& t ) : image ( std::move ( t.image ) )
	{
		std::swap ( imageView, t.imageView );
	}
	Texture ( const Texture& ) = delete;
	~Texture () 
	{
		clean ();
	}
	
	Texture& operator = ( Texture& ) = delete;

	bool	isOk () const
	{
		return image.isOk () && imageView != VK_NULL_HANDLE;
	}

	void	clean ()
	{
		image.clean ();
		
		if ( imageView != VK_NULL_HANDLE )
			vkDestroyImageView ( image.getDevice ()->getDevice (), imageView, nullptr );

		imageView = VK_NULL_HANDLE;
	}

	Image&	getImage ()
	{
		return image;
	}

	VkImageView	getImageView () const
	{
		return imageView;
	}

	uint32_t	getWidth () const
	{
		return image.getWidth ();
	}

	uint32_t	getHeight () const
	{
		return image.getHeight ();
	}

	uint32_t	getDepth () const
	{
		return image.getDepth ();
	}

	VkFormat	getFormat () const
	{
		return image.getFormat ();
	}

	void	createImageView ( VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D );

	Texture&	create ( Device& dev, uint32_t w, uint32_t h, uint32_t d, uint32_t mipLevels, VkFormat fmt, VkImageTiling tl, 
						 VkImageUsageFlags usage, int mapping, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
	
	void	generateMipmaps ( SingleTimeCommand& cmd, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels );
	bool	load            ( Device& dev, const std::string& fileName, bool mipmaps = true, bool srgb = false );
	bool	loadCubemap     ( Device& dev, const std::vector<const char *>& files, bool mipmaps = true, bool srgb = false );
	bool	loadRaw         ( Device& dev, int w, int h, const void * ptr, VkFormat format, bool mipmaps = true );
};

class	Data;

bool	loadDds ( Device& device, Texture& texture, Data& data, bool srgb = false );
