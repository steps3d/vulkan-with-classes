
#pragma once
#include	"Device.h"

class	SingleTimeCommand
{
	Device			  * device = nullptr;
	bool			syncOnExit = false;
	VkQueue			queue;
	VkCommandPool		commandPool;
	VkCommandBuffer		commandBuffer = VK_NULL_HANDLE;

public:
	SingleTimeCommand ( Device& dev, bool sync = true ) : device ( &dev ), queue ( dev.getGraphicsQueue () ), commandPool ( dev.getCommandPool () ), syncOnExit ( sync )
	{
		assert ( dev.getDevice () != VK_NULL_HANDLE );
		assert ( commandPool != VK_NULL_HANDLE );
		assert ( queue != VK_NULL_HANDLE );

		VkCommandBufferAllocateInfo	allocInfo = {};
		VkCommandBufferBeginInfo	beginInfo = {};
	
		allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool        = commandPool;
		allocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers ( device->getDevice (), &allocInfo, &commandBuffer );

		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer ( commandBuffer, &beginInfo );
	}

	SingleTimeCommand ( Device& dev, VkQueue q, VkCommandPool pool ) : device ( &dev ), queue ( q ), commandPool ( pool )
	{
		assert ( dev.getDevice () != VK_NULL_HANDLE );
		assert ( commandPool != VK_NULL_HANDLE );
		assert ( queue != VK_NULL_HANDLE );

		VkCommandBufferAllocateInfo	allocInfo = {};
		VkCommandBufferBeginInfo	beginInfo = {};
	
		allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool        = commandPool;
		allocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers ( device->getDevice (), &allocInfo, &commandBuffer );

		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer ( commandBuffer, &beginInfo );
	}

	~SingleTimeCommand ()
	{
		assert ( device->getDevice () != VK_NULL_HANDLE );
		assert ( commandPool != VK_NULL_HANDLE );
		assert ( queue != VK_NULL_HANDLE );
		assert ( commandBuffer != VK_NULL_HANDLE );

		vkEndCommandBuffer ( commandBuffer );

		VkSubmitInfo submitInfo = {};
	
		submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers    = &commandBuffer;

		VkFence fence               = VK_NULL_HANDLE;

		if ( syncOnExit )
		{
			VkFenceCreateInfo fenceInfo = {};

			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

			vkCreateFence ( device->getDevice (), &fenceInfo, nullptr, &fence );
		}

			// Submit to the queue
		vkQueueSubmit ( queue, 1, &submitInfo, fence );

			// Wait for the fence to signal that command buffer has finished executing
		if ( syncOnExit )
		{
			vkWaitForFences ( device->getDevice (), 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT );
			vkDestroyFence  ( device->getDevice (), fence, nullptr );
		}

//		vkQueueSubmit        ( queue, 1, &submitInfo, VK_NULL_HANDLE );
		vkQueueWaitIdle      ( queue );
		vkFreeCommandBuffers ( device->getDevice (), commandPool, 1, &commandBuffer );
	}

	VkCommandBuffer	getHandle () const
	{
		return commandBuffer;
	}
};
