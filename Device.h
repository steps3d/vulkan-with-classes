#pragma once

#define	USE_VMA	1

#ifdef USE_VMA
#include	<vk_mem_alloc.h>
#endif // USE_VMA

#ifdef _WIN32
#include	<Windows.h>
#endif

#include	<vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include	"Log.h"

#define DEFAULT_FENCE_TIMEOUT 100000000000
#define	GLM_FORCE_DEPTH_ZERO_TO_ONE

template <typename T>
inline T alignedSize ( T value, T alignment )
{
	return (value + alignment - 1) & ~(alignment - 1);
}

class	CommandBuffer;

struct QueueFamilyIndices		// class to hold indices to queue families
{
	enum
	{
		noValue = UINT32_MAX
	};

	uint32_t graphicsFamily = noValue;
	uint32_t presentFamily  = noValue;
	uint32_t computeFamily  = noValue;

	bool isComplete() const		// require all families are present
	{
		return (graphicsFamily != noValue) && (presentFamily != noValue) && (computeFamily != noValue);
	}

	static QueueFamilyIndices findQueueFamilies ( VkPhysicalDevice device, VkSurfaceKHR surface );
};

class	Device
{
	VkInstance							instance            = VK_NULL_HANDLE;	// vulkan instance for app
	VkDebugUtilsMessengerEXT			debugMessenger      = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT 			msgCallback         = VK_NULL_HANDLE;
	VkPhysicalDevice					physicalDevice      = VK_NULL_HANDLE;
	VkDevice							device              = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties2			properties          = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	VkPhysicalDeviceFeatures2			features            = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2   };
	VkPhysicalDeviceMemoryProperties	memoryProperties    = {};
	VkQueue								graphicsQueue       = VK_NULL_HANDLE;
	VkQueue								presentQueue        = VK_NULL_HANDLE;
	VkQueue								computeQueue        = VK_NULL_HANDLE;
	VkCommandPool						commandPool         = VK_NULL_HANDLE;
	QueueFamilyIndices					families;
	std::vector<VkExtensionProperties>	extensions;

#ifdef USE_VMA
	VmaAllocator						allocator           = VK_NULL_HANDLE;
#endif // USE_VMA

	friend class VulkanWindow;
	
public:
	Device  () {}
	Device ( const Device& ) = delete;
	Device ( Device&& dev )
	{
		std::swap ( instance,         dev.instance );
		std::swap ( debugMessenger,   dev.debugMessenger   );
		std::swap ( msgCallback,      dev.msgCallback      );
		std::swap ( physicalDevice,   dev.physicalDevice   );
		std::swap ( device,           dev.device           );
		std::swap ( properties,       dev.properties       );
		std::swap ( features,         dev.features         );
		std::swap ( memoryProperties, dev.memoryProperties );
		std::swap ( graphicsQueue,    dev.graphicsQueue    );
		std::swap ( presentQueue,     dev.presentQueue     );
		std::swap ( computeQueue,     dev.computeQueue     );
		std::swap ( commandPool,      dev.commandPool      );
		std::swap ( families,         dev.families         );
	}

	~Device () 
	{
		clean ();
	}

	Device& operator = ( const Device& ) = delete;

	bool	isOk () const
	{
		return  instance != VK_NULL_HANDLE && physicalDevice != VK_NULL_HANDLE && 
			    device != VK_NULL_HANDLE && graphicsQueue != VK_NULL_HANDLE && 
			    presentQueue != VK_NULL_HANDLE && commandPool != VK_NULL_HANDLE;
	}

	VkDevice	getDevice () const
	{
		return device;
	}
	
	VkInstance	getInstance () const
	{
		return instance;
	}
	
	VkPhysicalDevice	getPhysicalDevice () const
	{
		return physicalDevice;
	}
	
	VkQueue	getGraphicsQueue () const
	{
		return graphicsQueue;
	}
	
	VkQueue	getPresentQueue () const
	{
		return presentQueue;
	}
	
	VkQueue	getComputeQueue () const
	{
		return computeQueue;
	}
	
	uint32_t	getGraphicsFamilyIndex () const
	{
		return families.graphicsFamily;
	}
	
	uint32_t	getPresentFamilyIndex () const
	{
		return families.presentFamily;
	}
	
	uint32_t	getComputeFamilyIndex () const
	{
		return families.computeFamily;
	}
	
	VkCommandPool	getCommandPool () const
	{
		return commandPool;
	}

	const QueueFamilyIndices&	getFamilies () const
	{
		return families;
	}

	const VkPhysicalDeviceProperties2&	getProperties () const
	{
		return properties;
	}

	const VkPhysicalDeviceFeatures2&	getFeatures () const
	{
		return features;
	}

	const VkPhysicalDeviceMemoryProperties&	getMemoryProperties () const
	{
		return memoryProperties;
	}

#ifdef USE_VMA
	VmaAllocator	getAllocator () const
	{
		return allocator;
	}
#endif // USE_VMA

		// create logical device, get queues, create command pool
		// pNext is used in VkDeviceCreateInfo 
	bool	create ( VkInstance _instance, VkPhysicalDevice _physicalDebice, 
					 VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions, 
					 const std::vector<const char*>& validationLayers, void * pNextFeatures = nullptr, void * pNextProperties = nullptr );

	void	clean ()
	{
		if ( commandPool != VK_NULL_HANDLE )
			vkDestroyCommandPool ( device, commandPool, nullptr );

#ifdef USE_VMA
		if ( allocator )
			vmaDestroyAllocator ( allocator );
#endif
		allocator = VK_NULL_HANDLE;

		if ( device != VK_NULL_HANDLE )
			vkDestroyDevice ( device, nullptr );

		commandPool = VK_NULL_HANDLE;
		device      = VK_NULL_HANDLE;
	}

	void						freeCommandBuffer   ( CommandBuffer& buffer );
	std::vector<CommandBuffer>	allocCommandBuffers ( uint32_t count );
};