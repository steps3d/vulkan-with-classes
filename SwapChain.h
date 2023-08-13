
#include	"Log.h"
#include	"Device.h"
#include	"Pipeline.h"
#include	"Semaphore.h"

class	SwapChain
{
	struct SwapChainSupportDetails 
	{
		VkSurfaceCapabilitiesKHR        capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR>   presentModes;
	};

	GLFWwindow				  * window          = nullptr;
	Device					  * device          = nullptr;
	VkSurfaceKHR				surface         = VK_NULL_HANDLE;
	VkSwapchainKHR				swapChain       = VK_NULL_HANDLE;
	bool						useSrgb         = false;
	bool						vSync           = true;
	std::vector<VkImage>		swapChainImages;
	std::vector<VkImageView> 	swapChainImageViews;
	std::vector<VkFramebuffer>	swapChainFramebuffers;
	VkFormat					swapChainImageFormat;
	VkExtent2D					swapChainExtent;

					// sync objects
	std::vector<Semaphore>		imageAvailableSemaphores;
	std::vector<Semaphore>		renderFinishedSemaphores;
	std::vector<Fence>			inFlightFences;
	std::vector<VkFence>		imagesInFlight;			// just reference to inFlightFences
	size_t 						currentFrame = 0;

	const int MAX_FRAMES_IN_FLIGHT = 2;

public:
	SwapChain  () = default;
	~SwapChain () = default;

	VkFormat	getFormat () const
	{
		return swapChainImageFormat;
	}
	
	const VkExtent2D	getExtent () const
	{
		return swapChainExtent;
	}

	int	getWidth () const
	{
		return swapChainExtent.width;
	}

	int	getHeight () const
	{
		return swapChainExtent.height;
	}

	uint32_t	imageCount () const
	{
		return (uint32_t) swapChainImages.size ();
	}

	const std::vector<VkFramebuffer>& getFramebuffers () const
	{
		return swapChainFramebuffers;
	}

	const std::vector<VkImage>	getImages () const
	{
		return swapChainImages;
	}
	
	VkSemaphore	currentAvailableSemaphore () const
	{
		return imageAvailableSemaphores [currentFrame].getHandle ();
	}
	
	VkSemaphore	currentRenderFinishedSemaphore () const
	{
		return  renderFinishedSemaphores [currentFrame].getHandle ();
	}
	
	VkFence	currentInFlightFence () const
	{
		return inFlightFences [currentFrame].getHandle ();
	}
	
	void	clean ( bool cleanSync = true )
	{
		if ( cleanSync )
			for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
			{
				imageAvailableSemaphores [i].clean ();	
				renderFinishedSemaphores [i].clean ();
				inFlightFences           [i].clean ();
			}

		for ( auto framebuffer : swapChainFramebuffers )
			vkDestroyFramebuffer ( device->getDevice (), framebuffer, nullptr );

		for ( auto imageView : swapChainImageViews ) 
			vkDestroyImageView ( device->getDevice (), imageView, nullptr );
		
		vkDestroySwapchainKHR ( device->getDevice (), swapChain, nullptr );

		swapChainFramebuffers.clear ();
		swapChainImageViews.clear   ();

		swapChain = VK_NULL_HANDLE;
	}

	void create ( Device& dev, VkSurfaceKHR surf, GLFWwindow * win, int width, int height, bool srgb )
	{
		device  = &dev;
		window  = win;
		surface = surf;
		useSrgb = srgb;

		VkSurfaceCapabilitiesKHR surfCaps;
		SwapChainSupportDetails	swapChainSupport = querySwapChainSupport   ();
		VkSurfaceFormatKHR		surfaceFormat    = chooseSwapSurfaceFormat ( swapChainSupport.formats );
		VkPresentModeKHR		presentMode      = chooseSwapPresentMode   ( swapChainSupport.presentModes );
		VkExtent2D				extent           = chooseSwapExtent        ( swapChainSupport.capabilities, width, height );
		uint32_t				imageCount       = swapChainSupport.capabilities.minImageCount + 1;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR ( device->getPhysicalDevice (), surface, &surfCaps );

		if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount )
			imageCount = swapChainSupport.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface          = surface;
		createInfo.minImageCount    = imageCount;
		createInfo.imageFormat      = surfaceFormat.format;
		createInfo.imageColorSpace  = surfaceFormat.colorSpace;
		createInfo.imageExtent      = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.preTransform     = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode      = presentMode;
		createInfo.clipped          = VK_TRUE;
		createInfo.oldSwapchain     = VK_NULL_HANDLE;

				// enable transfer source on swap chain images if supported
		if ( surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT )
			createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

				// enable transfer destination on swap chain images if supported
		if ( surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
			createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		auto		indices              = device->getFamilies ();	
		uint32_t	queueFamilyIndices[] =  { indices.graphicsFamily, indices.presentFamily };

		if ( indices.graphicsFamily != indices.presentFamily )
		{
			createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices   = queueFamilyIndices;
		} 
		else
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if ( vkCreateSwapchainKHR ( device->getDevice (), &createInfo, nullptr, &swapChain ) != VK_SUCCESS )
			fatal () << "SwapChain: failed to create swap chain!";

			// get swap chain images
		vkGetSwapchainImagesKHR ( device->getDevice (), swapChain, &imageCount, nullptr );
		swapChainImages.resize  ( imageCount );
		vkGetSwapchainImagesKHR ( device->getDevice (), swapChain, &imageCount, swapChainImages.data () );

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent      = extent;

		createImageViews   ();
	}

	void createImageViews ()
	{
		swapChainImageViews.resize ( swapChainImages.size () );

		for ( size_t i = 0; i < swapChainImages.size (); i++ ) 
		{
			VkImageViewCreateInfo createInfo = {};

			createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image                           = swapChainImages[i];
			createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format                          = swapChainImageFormat;
			createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel   = 0;
			createInfo.subresourceRange.levelCount     = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount     = 1;

			if ( vkCreateImageView ( device->getDevice (), &createInfo, nullptr, &swapChainImageViews [i] ) != VK_SUCCESS )
				fatal () << "SwapChain: failed to create image views!";
		}
	}

	void createFramebuffers ( Renderpass& renderPass, VkImageView depthImageView = VK_NULL_HANDLE ) 
	{
		swapChainFramebuffers.resize ( swapChainImageViews.size () );

		for ( size_t i = 0; i < swapChainImageViews.size(); i++ ) 
		{
			VkImageView				attachments [] = { swapChainImageViews [i], depthImageView };
			VkFramebufferCreateInfo	framebufferInfo = {};
			
			framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass      = renderPass.getHandle ();
			framebufferInfo.attachmentCount = depthImageView != VK_NULL_HANDLE ? 2 : 1;
			framebufferInfo.pAttachments    = attachments;
			framebufferInfo.width           = swapChainExtent.width;
			framebufferInfo.height          = swapChainExtent.height;
			framebufferInfo.layers          = 1;

			if ( vkCreateFramebuffer ( device->getDevice (), &framebufferInfo, nullptr, &swapChainFramebuffers[i] ) != VK_SUCCESS )
				fatal () << "SwapChain: failed to create framebuffer!";
		}
	}

	void createFramebuffers ( Renderpass& renderPass, Texture& image ) 
	{
		createFramebuffers ( renderPass, image.getImageView () );
	}

	void createSyncObjects ()
	{
		imageAvailableSemaphores.resize ( MAX_FRAMES_IN_FLIGHT );
		renderFinishedSemaphores.resize ( MAX_FRAMES_IN_FLIGHT );
		inFlightFences.          resize ( MAX_FRAMES_IN_FLIGHT );
		imagesInFlight.          resize ( swapChainImages.size (), VK_NULL_HANDLE );

		for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			imageAvailableSemaphores [i].create ( *device );
			renderFinishedSemaphores [i].create ( *device );
			inFlightFences           [i].create ( *device, true );
		}
	}

	uint32_t	acquireNextImage ()
	{
		uint32_t	imageIndex;

		inFlightFences [currentFrame].wait ( UINT64_MAX );

				// check for recreation
		if ( vkAcquireNextImageKHR ( device->getDevice (), swapChain, UINT64_MAX, imageAvailableSemaphores [currentFrame].getHandle (), VK_NULL_HANDLE, &imageIndex ) ==  VK_ERROR_OUT_OF_DATE_KHR )
			return UINT32_MAX;

		if ( imagesInFlight[imageIndex] != VK_NULL_HANDLE )
			vkWaitForFences ( device->getDevice (), 1, &imagesInFlight [imageIndex], VK_TRUE, UINT64_MAX );
        
		imagesInFlight [imageIndex] = inFlightFences [currentFrame].getHandle ();
		
		return imageIndex;
	}

	void	present ( uint32_t imageIndex, VkQueue presentQueue )
	{
		VkPresentInfoKHR	presentInfo         = {};
		VkSwapchainKHR		swapChains       [] = { swapChain };
		VkSemaphore			signalSemaphores [] = { renderFinishedSemaphores [currentFrame].getHandle () };
		
		presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores    = signalSemaphores;	
		presentInfo.swapchainCount     = 1;
		presentInfo.pSwapchains        = swapChains;
		presentInfo.pImageIndices      = &imageIndex;

		vkQueuePresentKHR ( presentQueue, &presentInfo );

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	
private:
	VkSurfaceFormatKHR chooseSwapSurfaceFormat ( const std::vector<VkSurfaceFormatKHR>& availableFormats )
	{
		for ( const auto& availableFormat : availableFormats )
			if ( useSrgb && availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
				return availableFormat;
			else
			if ( !useSrgb && availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
				return availableFormat;

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode ( const std::vector<VkPresentModeKHR>& availablePresentModes )
	{
		if ( vSync )
		{
			for ( const auto& availablePresentMode : availablePresentModes )
				if ( availablePresentMode == VK_PRESENT_MODE_FIFO_KHR )
					return VK_PRESENT_MODE_FIFO_KHR;

			for ( const auto& availablePresentMode : availablePresentModes )
				if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
					return VK_PRESENT_MODE_MAILBOX_KHR;
		}
		else
		{
			for ( const auto& availablePresentMode : availablePresentModes )
				if ( availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR )
					return VK_PRESENT_MODE_IMMEDIATE_KHR;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent ( const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height )
	{
		if ( capabilities.currentExtent.width != UINT32_MAX )
			return capabilities.currentExtent;

		VkExtent2D actualExtent = { width, height };

		actualExtent.width  = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}

	SwapChainSupportDetails querySwapChainSupport ()
	{
		SwapChainSupportDetails details;
		uint32_t 		formatCount;
		uint32_t		presentModeCount;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR ( device->getPhysicalDevice (), surface, &details.capabilities );
		vkGetPhysicalDeviceSurfaceFormatsKHR      ( device->getPhysicalDevice (), surface, &formatCount, nullptr );

		if ( formatCount != 0 ) 
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR ( device->getPhysicalDevice (), surface, &formatCount, details.formats.data () );
		}

		vkGetPhysicalDeviceSurfacePresentModesKHR ( device->getPhysicalDevice (), surface, &presentModeCount, nullptr );

		if ( presentModeCount != 0 ) 
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR ( device->getPhysicalDevice (), surface, &presentModeCount, details.presentModes.data () );
		}

		return details;
	}
};
