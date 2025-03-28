#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <cstdint>

#include	"Device.h"
#include	"SwapChain.h"
#include	"Pipeline.h"
#include	"Texture.h"
#include	"CommandBuffer.h"
#include	"DescriptorSet.h"

class Buffer;
class Image;
class Controller;

struct	DevicePolicy
{
	VkPhysicalDeviceFeatures2					features                    = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	std::vector<const char*>					validationLayers            = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*>					deviceExtensions            = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkPhysicalDeviceProperties2			      * extraProperties             = nullptr;
	VkPhysicalDeviceBufferDeviceAddressFeatures	bufferDeviceAddressFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
	VkPhysicalDeviceVulkan13Features			features13                  = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	VkFormat					depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

	DevicePolicy ()
	{
		bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
		features13.dynamicRendering                     = VK_TRUE;
		features13.synchronization2                     = VK_TRUE;

		addFeatures ( &bufferDeviceAddressFeatures );
		addFeatures ( &features13 );
	}

	virtual ~DevicePolicy () = default;

	void	addDeviceExtension ( const char * extName )
	{
		deviceExtensions.push_back ( extName );
	}

	void	addValidationLayer ( const char * layerName )
	{
		validationLayers.push_back ( layerName );
	}

		// add new feature to the features.pNext list
	void	addFeatures ( void * pFeatures )
	{
		static_cast<VkPhysicalDeviceFeatures2 *>(pFeatures)->pNext = features.pNext;

		features.pNext = pFeatures;
	}

		// add property to the list of properties
	void	addProperties ( void * pProperties )
	{
		if ( extraProperties != nullptr )
			static_cast<VkPhysicalDeviceProperties2 *>(pProperties)->pNext = extraProperties -> pNext;

		extraProperties = static_cast<VkPhysicalDeviceProperties2 *>( pProperties );
	}
	
	bool supportsPresentation ( VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex ) const
	{
		return  glfwGetPhysicalDevicePresentationSupport ( instance, physicalDevice, queueFamilyIndex ) == GLFW_TRUE;
	}

		// pick appropriate physical device
	virtual VkPhysicalDevice	pickDevice ( VkInstance instance, std::vector<VkPhysicalDevice>& devices, VkSurfaceKHR surface ) const
	{
		VkPhysicalDevice			preferred = VK_NULL_HANDLE;
		VkPhysicalDevice			fallback  = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties	props;

		for ( const auto& dev : devices )								// check every device
		{
			vkGetPhysicalDeviceProperties ( dev, &props );				// get device properties

			if ( isDeviceSuitable ( instance, dev, surface, props ) )	// use surface and checks for queue families
			{
				if ( !preferred && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
					preferred = dev;

				if ( !fallback )
					fallback = dev;
			}
		}

		VkPhysicalDevice	device = preferred ? preferred : fallback;

		if ( device )
		{
			vkGetPhysicalDeviceProperties ( device, &props );				// get device properties

			log () << "GPU " <<  props.deviceName << std::endl;

			return device;
		}

		return VK_NULL_HANDLE;
	}

		// check whether this device is ok
	virtual bool	isDeviceSuitable ( VkInstance instance, VkPhysicalDevice device, VkSurfaceKHR surface, VkPhysicalDeviceProperties& props ) const
	{
		QueueFamilyIndices			indices             = QueueFamilyIndices::findQueueFamilies ( device, surface );
		uint32_t					graphicsFamilyIndex = indices.graphicsFamily;

		if ( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU )
			return false;

		if ( graphicsFamilyIndex == VK_QUEUE_FAMILY_IGNORED )
			return false;

		if ( !supportsPresentation ( instance, device, graphicsFamilyIndex ) )
			return false;

		if ( props.apiVersion < VK_API_VERSION_1_1 )
			return false;

		return indices.isComplete ();
	}

		// create device, picking required features
	virtual	void	createDevice ( VkInstance instance, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, Device& device )
	{
		device.create ( instance, physicalDevice, surface, deviceExtensions, validationLayers, &features, extraProperties );
	}
};

class	VulkanWindow
{
protected:
	enum
	{
		MAX_FPS_FRAMES = 5								// maximum number of frames we use for FPS computing
	};

	std::string			appName      = "Vulkan application";
	std::string			engineName   = "No engine";
	DevicePolicy      * policy       = nullptr;
	GLFWwindow    	  * window       = nullptr;
	bool				hasDepth     = true;			// do we should attach depth buffer to FB
	bool				srgb         = false;
	uint32_t			currentImage = 0;
	Controller		  * controller   = nullptr;
	bool				showFps      = false;
	bool				fullScreen   = false;
	int					frame        = 0;				// current frame number
	float				frameTime [MAX_FPS_FRAMES];		// time at last 5 frames for FPS calculations
	float				fps;
	int					savePosX, savePosY;				// save pos & size when going fullscreen
	int					saveWidth, saveHeight;	
	int					width, height;
	std::string			title;
	std::string			assetPath;						// path to various assets

	Device							device;
	VkDebugUtilsMessengerEXT		debugMessenger = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT 		msgCallback    = VK_NULL_HANDLE;
	VkSurfaceKHR					surface        = VK_NULL_HANDLE;
	VkInstance						instance       = VK_NULL_HANDLE;
	SwapChain						swapChain;
	Texture							depthTexture;		// may be empty
	DescriptorAllocator				descAllocator;

public:
	VulkanWindow ( int w, int h, const std::string& t, bool depth = true, DevicePolicy * p = nullptr ) : hasDepth ( depth )
	{
		if ( p == nullptr )
			p = new DevicePolicy ();

		initWindow ( w, h, t );
		initVulkan ( policy = p );
	}

	~VulkanWindow ()
	{
		descAllocator.clean ();
		depthTexture.clean  ();
		clean ();
	}

	void	setCaption ( const std::string& t )
	{
		title = t;
		glfwSetWindowTitle ( window, title.c_str () );
	}
	
	std::string	getCaption () const
	{
		return title;
	}
	
	double	getTime () const	// return time in seconds 
	{
		return glfwGetTime ();
	}

	GLFWwindow * getWindow () const
	{
		return window;
	}

	uint32_t	getWidth () const
	{
		return swapChain.getExtent ().width;
	}
	
	uint32_t	getHeight () const
	{
		return swapChain.getExtent ().height;
	}

	VkExtent2D	getExtent () const
	{
		return swapChain.getExtent ();
	}

	SwapChain&	getSwapChain ()
	{
		return swapChain;
	}

	DescriptorAllocator&	getDescriptorAllocator ()
	{
		return descAllocator;
	}
	
	Texture&	getDepthTexture ()
	{
		return depthTexture;
	}

	void	setSize ( uint32_t w, uint32_t h )
	{
		glfwSetWindowSize ( window, width = w, height = h );
	}
	
	float	getAspect () const
	{
		return static_cast<float> ( getWidth () ) / static_cast<float> ( getHeight () );
	}

	void	setShowFps ( bool flag )
	{
		showFps = flag;
	}
	
	bool	isFullScreen () const
	{
		return fullScreen;
	}

	const std::string	getAssetsPath () const
	{
		return assetPath;
	}

	void	setAssetPath ( const std::string& path )
	{
		assetPath = path;
	}

	Controller * getController ()
	{
		return controller;
	}

	void	setController ( Controller * ctrl )
	{
		controller = ctrl;
	}

	void	setFullScreen      ( bool flag );
	void	createDepthTexture ();
	bool	makeScreenshot     ( const std::string& fileName );
	void	defaultSubmit      ( CommandBuffer& cb );

			// create default renderpass with swapchain attachment
			// and depth attachments
	void	createDefaultRenderPass ( Renderpass& renderPass );

			// create window for rendering, called in c-tor
	void initWindow ( int w, int h, const std::string& t );

			// initialize Vulkan objects, called in c-tor
	void initVulkan ( DevicePolicy * );

			// clean all Vulkan objects
	virtual	void clean ();

			// main loop
	virtual int  run ();

			// cleanup swap chain objects due to window resize
	void cleanupSwapChain ()
	{
		depthTexture.clean ();
		
				// clean up objects in upper classes
		freePipelines ();

				// swapChain.cleanup without destroying sync objects
		swapChain.clean ( false );
	}
			// recreate swap chain objects due to window resize
	VkPhysicalDevice pickPhysicalDevice ();

	virtual void recreateSwapChain  ();
	virtual	void createInstance     ();
	virtual	void drawFrame          ();

		// perform actual sumitting of rendering 
	virtual	void submit ( uint32_t imageIndex ) {}


				// create pipelines, renderpasses, command buffers 
				// and descriptor sets
				// called on window size change
	virtual	void	createPipelines () {}
	
				// free them when close or change window size
	virtual	void	freePipelines   () {}

				// window events
	virtual	void	reshape     ( int w, int h ) {}
	virtual	void	keyTyped    ( int key, int scancode, int action, int mods );
	virtual	void	mouseMotion ( double x, double y );
	virtual	void	mouseClick  ( int button, int action, int mods );
	virtual	void	mouseWheel  ( double xOffset, double yOffset );
	virtual	void	idle        ();
	
protected:
	void	updateFps ();
	
			// get a list of required extensions
	std::vector<const char*> getRequiredInstanceExtensions () const;
	std::vector<const char*> getRequiredDeviceExtensions   () const;

			// check whether we support all required validation layers
	bool	checkValidationLayerSupport () const;

	virtual	void setupDebugMessenger ( VkInstance instance );
	virtual	void populateDebugMessengerCreateInfo ( VkDebugUtilsMessengerCreateInfoEXT& createInfo );

		// Vulkan debug callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback ( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
	{
		if ( messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )		// display only warning or higher
			log () << "validation layer: " << pCallbackData->pMessage << Log::endl;

		return VK_FALSE;
	}
	
			// Vulkan funcions from exts
	static VkResult createDebugUtilsMessengerEXT  ( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger );
	static void		destroyDebugUtilsMessengerEXT ( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator );
};

		// return normal matrix for a given model-view
inline glm::mat4 normalMatrix ( const glm::mat4& modelView ) 
{
	return glm::inverseTranspose ( modelView );
}

inline glm::mat4 projectionMatrix ( float fov, float aspect, float zMin, float zMax )
{
	glm::mat4	proj = glm::perspective ( glm::radians ( fov ), aspect, zMin, zMax );
	
	proj [1][1] *= -1;
	
	return proj;
}
