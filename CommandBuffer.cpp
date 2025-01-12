#include	"CommandBuffer.h"

CommandBuffer&	CommandBuffer :: create ( Device& dev )
{
	device = &dev;

	VkCommandBufferAllocateInfo		allocInfo = {};

	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool        = device->getCommandPool ();
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if ( vkAllocateCommandBuffers ( device->getDevice (), &allocInfo, &buffer ) != VK_SUCCESS )
		fatal () << "VulkanWindow: failed to allocate command buffers!";

	return *this;
}

CommandBuffer&	CommandBuffer :: begin ( bool simultenous )
{
	VkCommandBufferBeginInfo beginInfo = {};

	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if ( simultenous )
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if ( vkBeginCommandBuffer ( buffer, &beginInfo ) != VK_SUCCESS )
		fatal () << "VulkanWindow: failed to begin recording command buffer!";

	return *this;
}

CommandBuffer&	CommandBuffer :: end ()
{
	if ( hasRenderPass )
		vkCmdEndRenderPass ( buffer );

	if ( vkEndCommandBuffer ( buffer ) != VK_SUCCESS )
		fatal () << "VulkanWindow: failed to record command buffer!";

	return *this;
}

bool	SubmitInfo :: submit ( VkQueue queue, VkFence fence )
{
	submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount   = (uint32_t) waitSemaphores.size ();
	submitInfo.pWaitSemaphores      = waitSemaphores.data ();
	submitInfo.pWaitDstStageMask    = stageFlags.data ();
	submitInfo.commandBufferCount   = (uint32_t) commandBuffers.size ();
	submitInfo.pCommandBuffers      = commandBuffers.data ();
	submitInfo.signalSemaphoreCount = (uint32_t) signalSemaphores.size ();
	submitInfo.pSignalSemaphores    = signalSemaphores.data ();

	return vkQueueSubmit ( queue, 1, &submitInfo, fence ) == VK_SUCCESS;
}

VkImageMemoryBarrier2 imageBarrier ( VkImage image, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout, VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount )
{
	VkImageMemoryBarrier2 result = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };

	result.srcStageMask                  = srcStageMask;
	result.srcAccessMask                 = srcAccessMask;
	result.dstStageMask                  = dstStageMask;
	result.dstAccessMask                 = dstAccessMask;
	result.oldLayout                     = oldLayout;
	result.newLayout                     = newLayout;
	result.srcQueueFamilyIndex           = VK_QUEUE_FAMILY_IGNORED;
	result.dstQueueFamilyIndex           = VK_QUEUE_FAMILY_IGNORED;
	result.image                         = image;
	result.subresourceRange.aspectMask   = aspectMask;
	result.subresourceRange.baseMipLevel = baseMipLevel;
	result.subresourceRange.levelCount   = levelCount;
	result.subresourceRange.layerCount   = VK_REMAINING_ARRAY_LAYERS;

	return result;
}

VkBufferMemoryBarrier2 bufferBarrier ( VkBuffer buffer, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask )
{
	VkBufferMemoryBarrier2 result = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };

	result.srcStageMask        = srcStageMask;
	result.srcAccessMask       = srcAccessMask;
	result.dstStageMask        = dstStageMask;
	result.dstAccessMask       = dstAccessMask;
	result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.buffer              = buffer;
	result.offset              = 0;
	result.size                = VK_WHOLE_SIZE;

	return result;
}

void pipelineBarrier ( CommandBuffer& cb, std::initializer_list<VkBufferMemoryBarrier2> bufBarriers, VkDependencyFlags dependencyFlags )
{
	std::vector<VkBufferMemoryBarrier2>	bufferBarriers;
	VkDependencyInfo					dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };

	for ( auto& b : bufBarriers )
		bufferBarriers.push_back ( b );

	dependencyInfo.dependencyFlags          = dependencyFlags;
	dependencyInfo.bufferMemoryBarrierCount = uint32_t ( bufferBarriers.size () );
	dependencyInfo.pBufferMemoryBarriers    = bufferBarriers.data ();

	vkCmdPipelineBarrier2 ( cb.getHandle (), &dependencyInfo );
}

void pipelineBarrier ( CommandBuffer& cb, std::initializer_list<VkImageMemoryBarrier2> imgBarriers, VkDependencyFlags dependencyFlags )
{
	std::vector<VkImageMemoryBarrier2>	imageBarriers;
	VkDependencyInfo					dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };

	for ( auto& i : imgBarriers )
		imageBarriers.push_back ( i );

	dependencyInfo.dependencyFlags          = dependencyFlags;
	dependencyInfo.imageMemoryBarrierCount  = uint32_t ( imageBarriers.size () );
	dependencyInfo.pImageMemoryBarriers     = imageBarriers.data ();

	vkCmdPipelineBarrier2 ( cb.getHandle (), &dependencyInfo );
}

void pipelineBarrier ( CommandBuffer& cb, std::initializer_list<VkBufferMemoryBarrier2> bufBarriers, std::initializer_list<VkImageMemoryBarrier2>& imgBarriers, VkDependencyFlags dependencyFlags )
{
	std::vector<VkBufferMemoryBarrier2>	bufferBarriers;
	std::vector<VkImageMemoryBarrier2>	imageBarriers;
	VkDependencyInfo					dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };

	for ( auto& b : bufBarriers )
		bufferBarriers.push_back ( b );

	for ( auto& i : imgBarriers )
		imageBarriers.push_back ( i );

	dependencyInfo.dependencyFlags          = dependencyFlags;
	dependencyInfo.bufferMemoryBarrierCount = uint32_t ( bufferBarriers.size () );
	dependencyInfo.pBufferMemoryBarriers    = bufferBarriers.data ();
	dependencyInfo.imageMemoryBarrierCount  = uint32_t ( imageBarriers.size () );
	dependencyInfo.pImageMemoryBarriers     = imageBarriers.data ();

	vkCmdPipelineBarrier2 ( cb.getHandle (), &dependencyInfo );
}
