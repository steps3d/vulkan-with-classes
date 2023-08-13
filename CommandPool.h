
#pragma once

class	CommandPool
{
	Device		  * device = nullptr;
	VkCommandPool	pool   = VK_NULL_HANDLE;

public:
	CommandPool  () = default;
	CommandPool  ( CommandPool&& p )
	{
		std::swap ( device,  p.device );
		std::swap ( pool,    p.pool   );
	}
	CommandPool ( const CommandPool& ) = delete;
	~CommandPool ()
	{
		clean ();
	}

	CommandPool& operator = ( const CommandPool& ) = delete;

	VkCommandPool	getHandle () const
	{
		return pool;
	}

	void	clean ()
	{
		vkDestroyCommandPool ( device->getDevice (), pool, nullptr );
		
		pool = VK_NULL_HANDLE;
	}

	void	create ( Device& dev, bool graphics = true, VkCommandPoolCreateFlagBits flags = (VkCommandPoolCreateFlagBits)0 )
	{
		device = &dev;

		VkCommandPoolCreateInfo	poolInfo           = {};
		
		poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = device->getGraphicsFamilyIndex ();
		poolInfo.flags            = flags;

		if ( !graphics )
			poolInfo.queueFamilyIndex = device->getComputeFamilyIndex ();

		if ( vkCreateCommandPool ( device->getDevice (), &poolInfo, nullptr, &pool ) != VK_SUCCESS )
			fatal () << "CommandPool: failed to create command pool!" << Log::endl;
	}

	void	alloc ( std::vector<VkCommandBuffer>& commandBuffers, uint32_t n )
	{
		commandBuffers.resize ( n );

		VkCommandBufferAllocateInfo allocInfo = {};

		allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool        = pool;
		allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = n;

		if ( vkAllocateCommandBuffers ( device->getDevice (), &allocInfo, commandBuffers.data () ) != VK_SUCCESS )
			fatal () << "CommandPool: failed to allocate command buffers!";
	}

	VkCommandBuffer	alloc ()
	{
		VkCommandBufferAllocateInfo	allocInfo = {};
		VkCommandBuffer			cmd       = VK_NULL_HANDLE;

		allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool        = pool;
		allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if ( vkAllocateCommandBuffers ( device->getDevice (), &allocInfo, &cmd ) != VK_SUCCESS )
			fatal () << "CommandPool: failed to allocate command buffers!";

		return cmd;
	}
};
