
#pragma once

class	Semaphore 
{
	VkSemaphore handle = VK_NULL_HANDLE;
	Device   * device  = nullptr;

public:
	Semaphore  () = default;
	Semaphore  ( Semaphore&& s )
	{
		std::swap ( handle, s.handle );
	}
	Semaphore ( const Semaphore& ) = delete;
	~Semaphore ()
	{
		clean ();
	}

	Semaphore& operator = ( const Semaphore& ) = delete;	

	bool	isOk () const
	{
		return device != nullptr && handle != VK_NULL_HANDLE;
	}

	VkSemaphore	getHandle () const
	{
		return handle;
	}

	void	clean ()
	{
		if ( handle != VK_NULL_HANDLE )
			vkDestroySemaphore ( device->getDevice (), handle, nullptr );

		handle = VK_NULL_HANDLE;
	}

	void	create ( Device& dev )
	{
		device = &dev;

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};

		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if ( vkCreateSemaphore ( device->getDevice (), &semaphoreCreateInfo, nullptr, &handle ) != VK_SUCCESS )
			fatal () << "Semaphore: error creating" << Log::endl;
	}


	void	signal ( VkQueue queue )
	{
		VkSubmitInfo	submitInfo = {};
		
		submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores    = &handle;
		
		vkQueueSubmit   ( queue, 1, &submitInfo, VK_NULL_HANDLE );
		vkQueueWaitIdle ( queue );
	}
};

class	Fence
{
	VkFence		fence               = VK_NULL_HANDLE;
	Device    * device  = nullptr;
	
public:
	Fence () = default;
	Fence ( Fence&& f )
	{
		std::swap ( fence, f.fence );
	}
	Fence ( const Fence& ) = delete;
	~Fence ()
	{
		clean ();
	}
	
	Fence& operator = ( const Fence& ) = delete;

	bool	isOk () const
	{
		return device != nullptr && fence != VK_NULL_HANDLE;
	}

	VkFence	getHandle () const
	{
		return fence;
	}
	
	void	clean ()
	{
		if ( fence != VK_NULL_HANDLE )
			vkDestroyFence  ( device->getDevice (), fence, nullptr );

		fence = VK_NULL_HANDLE;
	}
	
	void	create ( Device& dev, bool signaled = false )
	{
		VkFenceCreateInfo fenceInfo = {};

		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
		device          = &dev;
		
		if ( vkCreateFence ( device->getDevice (), &fenceInfo, nullptr, &fence ) != VK_SUCCESS )
			fatal () << "Fence: error creating" << Log::endl;
	}

		// set state to unsignalled from host
	void	reset ()
	{
		vkResetFences ( device->getDevice (), 1, &fence );
	}

		// VK_SUCCESS - signaled
		// VK_NOT_READY unsignaled
	VkResult	status () const
	{
		return vkGetFenceStatus ( device->getDevice (), fence );
	}

		// wait for fence, timeout in nanoseconds
	bool	wait ( uint64_t timeout )
	{
		return vkWaitForFences ( device->getDevice (), 1, &fence, VK_TRUE, timeout ) == VK_SUCCESS;
	}
};

class	Event
{
	VkEvent		event               = VK_NULL_HANDLE;
	Device    * device  = nullptr;
	
public:
	Event () = default;
	Event( Event&& f )
	{
		std::swap ( event, f.event );
	}
	Event ( const Event& ) = delete;
	~Event ()
	{
		clean ();
	}
	
	Event& operator = ( const Event& ) = delete;

	bool	isOk () const
	{
		return device != nullptr && event != VK_NULL_HANDLE;
	}

	VkEvent	getHandle () const
	{
		return event;
	}
	
	void	clean ()
	{
		if ( event != VK_NULL_HANDLE )
			vkDestroyEvent  ( device->getDevice (), event, nullptr );

		event = VK_NULL_HANDLE;
	}
	
	void	create ( Device& dev )
	{
		VkEventCreateInfo eventInfo = {};

		eventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
		device          = &dev;
		
		if ( vkCreateEvent ( device->getDevice (), &eventInfo, nullptr, &event ) != VK_SUCCESS )
			fatal () << "Event: error creating" << Log::endl;
	}

	bool	status () const
	{
		VkResult res = vkGetEventStatus ( device->getDevice (), event );

		return res == VK_EVENT_SET;
	}
		// set state to unsignalled from host
	void	reset ()
	{
		vkResetEvent ( device->getDevice (), event );
	}

	void	signal ()		// set
	{
		vkSetEvent ( device->getDevice (), event );
	}
};
