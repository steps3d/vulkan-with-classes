#define		VMA_IMPLEMENTATION
#define		VMA_STATIC_VULKAN_FUNCTIONS	 1
#define		VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include	"Device.h"
#include	"CommandBuffer.h"

QueueFamilyIndices QueueFamilyIndices::findQueueFamilies ( VkPhysicalDevice device, VkSurfaceKHR surface )
{
	QueueFamilyIndices	indices;
	uint32_t			queueFamilyCount = 0;
	int 				i                = 0;

	vkGetPhysicalDeviceQueueFamilyProperties ( device, &queueFamilyCount, nullptr );

	std::vector<VkQueueFamilyProperties> queueFamilies ( queueFamilyCount );

	vkGetPhysicalDeviceQueueFamilyProperties ( device, &queueFamilyCount, queueFamilies.data () );

	for ( const auto& queueFamily : queueFamilies )
	{
		if ( indices.graphicsFamily == QueueFamilyIndices::noValue && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
			indices.graphicsFamily = i;

		if ( indices.computeFamily == QueueFamilyIndices::noValue && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT )
			indices.computeFamily = i;

		VkBool32 presentSupport = false;

		if ( surface != nullptr )
			vkGetPhysicalDeviceSurfaceSupportKHR ( device, i, surface, &presentSupport );

		if ( presentSupport  )
			indices.presentFamily = i;

		if ( indices.isComplete () )
			return indices;

		i++;
	}

	return indices;
}

bool	Device :: create ( VkInstance _instance, VkPhysicalDevice _physicalDebice, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions, const std::vector<const char*>& validationLayers, void * pNextFeatures, void * pNextProperties )
{
	instance         = _instance;
	physicalDevice   = _physicalDebice;
	families         = QueueFamilyIndices::findQueueFamilies ( physicalDevice, surface );
	properties.pNext = pNextProperties;

		// get various properties of physical device
	vkGetPhysicalDeviceProperties2      ( physicalDevice, &properties       );
	vkGetPhysicalDeviceFeatures2        ( physicalDevice, &features         );
	vkGetPhysicalDeviceMemoryProperties ( physicalDevice, &memoryProperties );

	uint32_t	propertyCount;

	vkEnumerateDeviceExtensionProperties ( physicalDevice, nullptr, &propertyCount, nullptr );

	extensions.resize ( propertyCount );

	vkEnumerateDeviceExtensionProperties ( physicalDevice, nullptr, &propertyCount, extensions.data () );

	VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	float queuePriority                     = 1.0f;
	VkDeviceCreateInfo createInfo           = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };

	queueCreateInfo.queueFamilyIndex = families.graphicsFamily;
	queueCreateInfo.queueCount       = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;	

	createInfo.pQueueCreateInfos       = &queueCreateInfo;
	createInfo.queueCreateInfoCount    = 1;
	//createInfo.pEnabledFeatures        = &features;
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledLayerCount       = 0;
	createInfo.pNext                   = pNextFeatures;

	if ( vkCreateDevice ( physicalDevice, &createInfo, nullptr, &device ) != VK_SUCCESS )
		fatal () << "VulknaWindow: failed to create logical device!";

	vkGetDeviceQueue ( device, families.graphicsFamily, 0, &graphicsQueue );
	vkGetDeviceQueue ( device, families.presentFamily, 0, &presentQueue  );
	vkGetDeviceQueue ( device, families.computeFamily, 0, &computeQueue  );

		// create command pool
	VkCommandPoolCreateInfo	poolInfo           = {};

	poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = families.graphicsFamily;

	if ( vkCreateCommandPool ( getDevice (), &poolInfo, nullptr, &commandPool ) != VK_SUCCESS )
		fatal () << "VulkanWindow: failed to create command pool!" << Log::endl;

#ifdef	USE_VMA
	VmaAllocatorCreateInfo	allocatorCreateInfo = {};
	VmaVulkanFunctions		vulkanFuncs         = {};

	vulkanFuncs.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
	vulkanFuncs.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
	allocatorCreateInfo.physicalDevice   = physicalDevice;
	allocatorCreateInfo.device           = device;
	allocatorCreateInfo.instance         = instance;
	allocatorCreateInfo.flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	if ( vmaCreateAllocator ( &allocatorCreateInfo, &allocator ) != VK_SUCCESS )
		fatal () << "vmasCreateAllocator failure" << std::endl;
#endif
	return true;
}

std::vector<CommandBuffer> Device::allocCommandBuffers ( uint32_t count )
{
	std::vector<CommandBuffer>	buffers ( count );

	for ( auto& b : buffers )
		b.create ( *this );

	return buffers;
}

void	Device :: freeCommandBuffer ( CommandBuffer& buffer )
{
	VkCommandBuffer	buf = buffer.getHandle ();

	vkFreeCommandBuffers ( device, commandPool, 1, &buf );
}
