#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"Texture.h"
#include	"Controller.h"
#include	"stb_image_write.h"

const bool enableValidationLayers = true;

	// GLFW callbacks
static void _key ( GLFWwindow * window, int key, int scanCode, int action, int mods )
{
	VulkanWindow * win = (VulkanWindow *) glfwGetWindowUserPointer ( window );
		
	if ( win != nullptr )
		win -> keyTyped ( key, scanCode, action, mods );
	else
	if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
		glfwSetWindowShouldClose ( window, GLFW_TRUE );
}

static void	_reshape ( GLFWwindow * window, int w, int h )
{
	VulkanWindow * win = (VulkanWindow *) glfwGetWindowUserPointer (window );
		
	if ( win != nullptr )
		win -> reshape ( w, h );
}
	
static void _mouse ( GLFWwindow* window, double xpos, double ypos )
{
	VulkanWindow * win = (VulkanWindow *) glfwGetWindowUserPointer (window );
		
	if ( win != nullptr )
		win -> mouseMotion ( xpos, ypos );
}

static void _mouseClick ( GLFWwindow* window, int button, int action, int mods )
{
	VulkanWindow * win = (VulkanWindow *) glfwGetWindowUserPointer (window );
		
	if ( win != nullptr )
		win -> mouseClick ( button, action, mods );
}

static void _mouseScroll ( GLFWwindow* window, double xoffset, double yoffset )
{
	VulkanWindow * win = (VulkanWindow *) glfwGetWindowUserPointer (window );
		
	if ( win != nullptr )
		win -> mouseWheel ( xoffset, yoffset );
}	

////////////////////// VulkanWindow methods ///////////////////////////////

void VulkanWindow::initWindow ( int w, int h, const std::string& t )
{
	if ( !glfwInit () )
		fatal () << "VulkanWindow: error initializing GLFW" << Log::endl;
		
	glfwWindowHint ( GLFW_CLIENT_API, GLFW_NO_API );
//	glfwWindowHint ( GLFW_RESIZABLE,  GLFW_FALSE  );

	width  = w;
	height = h;
	title  = t;
	window = glfwCreateWindow ( width, height, title.c_str (), nullptr, nullptr );

				// set window callbacks
	glfwSetWindowUserPointer   ( window, this         );
	glfwSetKeyCallback         ( window, _key         );
	glfwSetWindowSizeCallback  ( window, _reshape     );
	glfwSetCursorPosCallback   ( window, _mouse       );	
	glfwSetMouseButtonCallback ( window, _mouseClick  );
	glfwSetScrollCallback      ( window, _mouseScroll );
}

void	VulkanWindow::initVulkan ( DevicePolicy * p )
{
	if ( policy == nullptr )
		fatal () << "No valid policy in initVulan !" << std::endl;

	if ( surface != VK_NULL_HANDLE )	// do no allow to initialize twice
		return;

	createInstance      ();
	setupDebugMessenger ( instance );
		
	if ( glfwCreateWindowSurface ( instance, window, nullptr, &surface ) != VK_SUCCESS )
		fatal () << "VulkanWindow: failed to create window surface!";

	policy->createDevice ( instance, pickPhysicalDevice (), surface, device );

	swapChain.create            ( device, surface, window, width, height, srgb );	
	swapChain.createSyncObjects ();
	descAllocator.create        ( device );

	createDepthTexture   ();
}

void	VulkanWindow::clean ()
{
	swapChain.clean    ();

	if ( enableValidationLayers )
		destroyDebugUtilsMessengerEXT ( device.getInstance (), debugMessenger, nullptr );
		
	vkDestroySurfaceKHR ( device.getInstance (), surface, nullptr );

	device.clean ();

	vkDestroyInstance ( device.getInstance (), nullptr );

	glfwDestroyWindow   ( window );
	glfwTerminate       ();
}

int	VulkanWindow::run () 
{
	//if ( device.getDevice () == VK_NULL_HANDLE )	// chec whether we've initialized Vulkan
	//	initVulkan ();	

	while ( !glfwWindowShouldClose ( window ) )
	{
		glfwPollEvents ();
		drawFrame      ();
		updateFps      ();
		idle           ();
	}

	vkDeviceWaitIdle ( device.getDevice () );

	return EXIT_SUCCESS;
}

void	VulkanWindow::recreateSwapChain ()
{
			// get new window size
	int width = 0, height = 0;
		
	while ( width == 0 || height == 0 )
	{
		glfwGetFramebufferSize ( window, &width, &height );
		glfwWaitEvents         ();
	}

			// wait till can do anything
	vkDeviceWaitIdle( device.getDevice () );

			// clean and recreate swap chain
	cleanupSwapChain ();

	swapChain.create ( device, surface, window, width, height, srgb );

			// create depth texture
	createDepthTexture ();
	
			// recreate pipelines, renderpasses and command buffers
	createPipelines ();
}

void	VulkanWindow::createInstance () 
{
	if ( enableValidationLayers && !checkValidationLayerSupport () )
		fatal () << "VulkanWindow: validation layers requested, but not available!";

	uint32_t 				glfwExtensionCount = 0;
	auto					extensions         = getRequiredInstanceExtensions();
	VkApplicationInfo		appInfo            = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	VkInstanceCreateInfo	createInfo         = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

	appInfo.pApplicationName   = appName.c_str ();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = engineName.c_str ();
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_3;

	createInfo.pApplicationInfo        = &appInfo;
	createInfo.enabledLayerCount       = 0;
	createInfo.enabledExtensionCount   = static_cast<uint32_t>( extensions.size () );
	createInfo.ppEnabledExtensionNames = extensions.data ();

	VkDebugUtilsMessengerCreateInfoEXT	debugCreateInfo;
	auto								validationLayers = policy->validationLayers;		// should be out of if !

	if ( enableValidationLayers ) 
	{
		populateDebugMessengerCreateInfo ( debugCreateInfo );

		createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.pNext               = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;		
	}

	if ( vkCreateInstance ( &createInfo, nullptr, &instance ) != VK_SUCCESS )
		fatal () << "VulkanWindow:failed to create instance!";
}

VkPhysicalDevice	VulkanWindow::pickPhysicalDevice ()
{
	uint32_t			deviceCount    = 0;
	VkPhysicalDevice	physicalDevice = VK_NULL_HANDLE;

	vkEnumeratePhysicalDevices ( instance, &deviceCount, nullptr );

	if ( deviceCount == 0 )
		fatal () << "VulkanWindow: failed to find GPUs with Vulkan support!";

	std::vector<VkPhysicalDevice> devices ( deviceCount );

	vkEnumeratePhysicalDevices ( instance, &deviceCount, devices.data () );

	if ( ( physicalDevice = policy->pickDevice ( instance, devices, surface ) ) == VK_NULL_HANDLE )
		fatal () << "VulkanWindow: failed to find a suitable GPU!" << std::endl;

	return physicalDevice;
}

void	VulkanWindow::setupDebugMessenger ( VkInstance instance )
{
	if ( !enableValidationLayers ) 
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

	populateDebugMessengerCreateInfo ( createInfo );

	if ( createDebugUtilsMessengerEXT ( instance, &createInfo, nullptr, &debugMessenger ) != VK_SUCCESS )
		fatal () << "VulkanWindow: failed to set up debug messenger!" << Log::endl;
}

void	VulkanWindow::createDepthTexture ()
{
	if ( hasDepth )
	{
		depthTexture.create ( device, getWidth (), getHeight (), 1, 1, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL, 
							  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 0 );
		
		
		SingleTimeCommand	cmd ( device, device.getGraphicsQueue (), device.getCommandPool () );
			
		depthTexture.getImage ().transitionLayout ( cmd, depthTexture.getImage ().getFormat (), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ); 
	}
}

void	VulkanWindow::createDefaultRenderPass ( Renderpass& renderPass )
{
	renderPass
		.addAttachment   ( swapChain.getFormat (),                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR )
		.addAttachment   ( depthTexture.getImage ().getFormat (), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
		.addSubpass      ( 0 )
		.addDepthSubpass ( 1 )
		.create          ( device );
}

void	VulkanWindow::drawFrame ()
{
			// wait till we're safe to submit
	currentImage = swapChain.acquireNextImage ();
		
	if ( currentImage == UINT32_MAX )		// we need to recreate swap chain
	{
		recreateSwapChain ();
			
		return;
	}
				// submit command buffers
	submit ( currentImage );
		
				// actually present image
	swapChain.present ( currentImage, device.getPresentQueue () );
}

std::vector<const char*> VulkanWindow::getRequiredInstanceExtensions () const
{
	uint32_t      glfwExtensionCount = 0;
	const char ** glfwExtensions     = glfwGetRequiredInstanceExtensions ( &glfwExtensionCount );
		
	std::vector<const char*> extensions ( glfwExtensions, glfwExtensions + glfwExtensionCount );

	if ( enableValidationLayers )
		extensions.push_back ( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

	//for ( auto ext : policy->instanceExtensions )
	//	extensions.push_back ( ext );

	return extensions;
}

std::vector<const char*> VulkanWindow::getRequiredDeviceExtensions () const
{
	uint32_t      glfwExtensionCount = 0;
	const char ** glfwExtensions     = glfwGetRequiredInstanceExtensions ( &glfwExtensionCount );

	std::vector<const char*> extensions ( glfwExtensions, glfwExtensions + glfwExtensionCount );

	if ( enableValidationLayers )
		extensions.push_back ( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

	for ( auto ext : policy->deviceExtensions )
		extensions.push_back ( ext );

	return extensions;
}

bool	VulkanWindow::checkValidationLayerSupport () const
{
	uint32_t layerCount;
		
	vkEnumerateInstanceLayerProperties ( &layerCount, nullptr );

	std::vector<VkLayerProperties> availableLayers ( layerCount );
		
	vkEnumerateInstanceLayerProperties ( &layerCount, availableLayers.data () );

	for ( const char * layerName : policy->validationLayers )
	{
		bool layerFound = false;

		for ( const auto& layerProperties : availableLayers )
		{
			if ( strcmp ( layerName, layerProperties.layerName ) == 0 )
			{
				layerFound = true;
				break;
			}
		}

		if ( !layerFound )
			return false;
	}

	return true;
}

void	VulkanWindow :: updateFps ()
{
	for ( int i = 0; i < MAX_FPS_FRAMES - 1; i++ )
		frameTime [i] = frameTime [i+1];
		
	frameTime [MAX_FPS_FRAMES - 1] = (float)getTime ();
	
														// add EPS to avoid zero division
	fps = MAX_FPS_FRAMES / (frameTime [MAX_FPS_FRAMES-1] - frameTime [0] + 0.00001f);
	frame++;
	
	if ( showFps && (frame % MAX_FPS_FRAMES == 0) )		// update FPS every 5'th frame
	{
		char buf [64];
		
		sprintf ( buf, "FPS: %5.1f ", fps );
		
		setCaption ( title + std::string( buf )  );
	}
}

void	VulkanWindow::setFullScreen ( bool flag )
{
	if ( flag == fullScreen )
        return;

    if ( flag )
    {
			// backup window position and window size
        glfwGetWindowPos  ( window, &savePosX,  &savePosY   );
        glfwGetWindowSize ( window, &saveWidth, &saveHeight );

			// get resolution of monitor
		GLFWmonitor       * monitor = glfwGetPrimaryMonitor ();
        const GLFWvidmode * mode    = glfwGetVideoMode      ( monitor );
	
			// switch to full screen
        glfwSetWindowMonitor( window, monitor, 0, 0, mode->width, mode->height, 0 );
    }
    else
    {
			// restore last window size and position
        glfwSetWindowMonitor ( window, nullptr,  savePosX, savePosY, saveWidth, saveHeight, 0 );
    }	
	
	fullScreen = flag;
}

bool	VulkanWindow::makeScreenshot ( const std::string& fileName )
{
	VkImage	srcImage = swapChain.getImages () [currentImage];		
	Image	image;

	image.create ( device, ImageParams ( getWidth (), getHeight () ).setFormat ( VK_FORMAT_R8G8B8A8_UNORM ).setTiling ( VK_IMAGE_TILING_LINEAR ).setUsage ( VK_IMAGE_USAGE_TRANSFER_DST_BIT ), 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	{
		SingleTimeCommand	cmd ( device );

		image.transitionLayout  ( cmd, image.getFormat (), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		Image::transitionLayout ( cmd, srcImage, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
			VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );

		// use image copy (requires us to manually flip components)
		VkImageCopy imageCopyRegion               = {};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width              = getWidth  ();
		imageCopyRegion.extent.height             = getHeight ();
		imageCopyRegion.extent.depth              = 1;

			// Issue the copy command
		vkCmdCopyImage(
			cmd.getHandle (),
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image.getHandle (), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &imageCopyRegion );

		image.transitionLayout  ( cmd, image.getFormat (), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL );
		Image::transitionLayout ( cmd, srcImage, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );
	}

	VkImageSubresource	subResource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout	subResourceLayout;

	vkGetImageSubresourceLayout ( device.getDevice (), image.getHandle (), &subResource, &subResourceLayout );

	const uint8_t * data = (const uint8_t *)image.map ();	//.getMemory ().map ();

	stbi_write_png ( fileName.c_str(), getWidth (), getHeight (), 4, data, 4*getWidth () );

	image.unmap ();		//.getMemory().unmap ();

	return true;
}

void	VulkanWindow::defaultSubmit ( CommandBuffer& cb )
{
	VkFence					currentFence        = swapChain.currentInFlightFence ();

	vkResetFences ( device.getDevice (), 1, &currentFence );

	SubmitInfo ()
		.wait    ( { { swapChain.currentAvailableSemaphore (), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT } } )
		.buffers ( { cb } )
		.signal  ( { swapChain.currentRenderFinishedSemaphore () } )
		.submit  ( device.getGraphicsQueue (), swapChain.currentInFlightFence () );
}

void	VulkanWindow::mouseMotion ( double x, double y )
{
	if ( controller )
		controller -> mouseMotion ( x, y );
}

void	VulkanWindow::mouseClick  ( int button, int action, int mods )
{
	if ( controller )
		controller -> mouseClick ( button, action, mods );
}

void	VulkanWindow::mouseWheel  ( double xOffset, double yOffset )
{
	if ( controller )
		controller -> mouseWheel ( xOffset, yOffset );
}

void	VulkanWindow::keyTyped ( int key, int scancode, int action, int mods )
{
	if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
		glfwSetWindowShouldClose ( window, GLFW_TRUE );		

	if ( key == GLFW_KEY_F1 && action == GLFW_PRESS )
		makeScreenshot ( "1.png" );

	if ( controller )
		controller -> keyTyped ( key, scancode, action, mods );
}

void	VulkanWindow::idle () 
{

}

 VkResult VulkanWindow::createDebugUtilsMessengerEXT ( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger )
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if ( func != nullptr )
		return func ( instance, pCreateInfo, pAllocator, pDebugMessenger );

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanWindow::destroyDebugUtilsMessengerEXT ( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator )
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if ( func != nullptr )
		func ( instance, debugMessenger, pAllocator );
}

void	VulkanWindow::populateDebugMessengerCreateInfo ( VkDebugUtilsMessengerCreateInfoEXT& createInfo )
{
	createInfo = {};

	createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}
