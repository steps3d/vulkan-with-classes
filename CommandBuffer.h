#pragma once

#include	"Device.h"
#include	"Pipeline.h"
#include	"DescriptorSet.h"
#include	"Framebuffer.h"
#include	"Semaphore.h"
//#include	"Log.h"

class	CommandBuffer;

class	RenderPassInfo
{
	VkRenderPassBeginInfo		renderPassInfo  = {};
	std::vector<VkClearValue>	clearValues;

public:
	RenderPassInfo ( Renderpass& renderPass ) 
	{
		renderPassInfo.sType      = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass.getHandle ();
	}

	VkRenderPassBeginInfo&	getInfo ()
	{
		renderPassInfo.clearValueCount = (uint32_t)clearValues.size ();
		renderPassInfo.pClearValues    = clearValues.data ();

		return renderPassInfo;
	}

	RenderPassInfo&	framebuffer ( Framebuffer& fb )
	{
		renderPassInfo.framebuffer = fb.getHandle ();

		return *this;
	}

	RenderPassInfo&	framebuffer ( VkFramebuffer fb )
	{
		renderPassInfo.framebuffer = fb;

		return *this;
	}

	RenderPassInfo& offset ( int x, int y )
	{
		renderPassInfo.renderArea.offset = {0, 0};

		return *this;
	}

	RenderPassInfo&	 extent ( int w, int h )
	{
		renderPassInfo.renderArea.extent.width  = w;
		renderPassInfo.renderArea.extent.height = h;

		return *this;
	}

	RenderPassInfo&	 extent ( const VkExtent2D& extent )
	{
		renderPassInfo.renderArea.extent = extent;

		return *this;
	}

	RenderPassInfo&	clearColor ( float r = 0, float g = 0, float b = 0, float a = 1 )
	{
		VkClearValue c;

		c.color = { r, g, b, a };

		clearValues.push_back ( c );

		return *this;
	}

	RenderPassInfo&	clearDepthStencil ( float d = 1.0f, uint32_t s = 0 )
	{
		VkClearValue c;

		c.depthStencil = { d, s };

		clearValues.push_back ( c );

		return *this;
	}
};

class Barrier
{
public:
	VkPipelineStageFlags				srcMask, dstMask;
	VkDependencyFlags					flags;
	std::vector<VkMemoryBarrier>		memoryBarriers;
	std::vector<VkBufferMemoryBarrier>	bufferMemoryBarriers;
	std::vector<VkImageMemoryBarrier>	imageMemoryBarriers;

	struct	Memory
	{
		VkAccessFlags      srcAccessMask;
		VkAccessFlags      dstAccessMask;

	public:
		Memory ( VkAccessFlags  src, VkAccessFlags dst ) : srcAccessMask ( src ), dstAccessMask ( dst ) {}
	};

	struct BufferMemory
	{
		VkAccessFlags      srcAccessMask;
		VkAccessFlags      dstAccessMask;
		uint32_t           srcQueueFamilyIndex;
		uint32_t           dstQueueFamilyIndex;
		VkBuffer           buffer;
		VkDeviceSize       offset;
		VkDeviceSize       size;	

		BufferMemory ( VkAccessFlags  src, VkAccessFlags dst,  uint32_t srcIndex, uint32_t dstIndex, Buffer& buf, VkDeviceSize offs = 0, VkDeviceSize sz = VK_WHOLE_SIZE ) 
		{
			srcAccessMask       = src;
			dstAccessMask       = dst;
			srcQueueFamilyIndex = srcIndex;
			dstQueueFamilyIndex = dstIndex;
			buffer              = buf.getHandle ();
			offs                = offs;
			size                = sz;
		}
	};

	struct ImageMemory
	{
		VkAccessFlags              srcAccessMask;
		VkAccessFlags              dstAccessMask;
		VkImageLayout              oldLayout;
		VkImageLayout              newLayout;
		uint32_t                   srcQueueFamilyIndex;
		uint32_t                   dstQueueFamilyIndex;
		VkImage                    image;
		VkImageSubresourceRange    subresourceRange;

		ImageMemory ( VkAccessFlags src, VkAccessFlags dst, VkImageLayout oldLyt, VkImageLayout newLyt, uint32_t srcIndex, uint32_t dstIndex, Image& img, VkImageSubresourceRange range )
		{
			srcAccessMask       = src;
			dstAccessMask       = dst;
			oldLayout           = oldLyt;
			newLayout           = newLyt;
			srcQueueFamilyIndex = srcIndex;
			dstQueueFamilyIndex = dstIndex;
			image               = img.getHandle ();
			subresourceRange    = range;
		}
	};

	Barrier ( VkPipelineStageFlags src, VkPipelineStageFlags dst, VkDependencyFlags dependencyFlags = 0 ) : srcMask ( src ), dstMask ( dst ), flags ( dependencyFlags ) {}

	VkPipelineStageFlags	srcStageMask () const
	{
		return srcMask;
	}

	VkPipelineStageFlags	dstStageMask () const
	{
		return dstMask;
	}

	VkDependencyFlags dependencyFlags () const
	{
		return flags;
	}

	Barrier&	barriers ( std::initializer_list<std::reference_wrapper<Memory>> bl )
	{
		for ( auto& s : bl )
			memoryBarriers.push_back ( { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, s.get ().srcAccessMask, s.get ().dstAccessMask } );

		return *this;
	}

	Barrier&	bufferBarriers ( std::initializer_list<std::reference_wrapper<BufferMemory>> bl )
	{
		for ( auto& s : bl )
			bufferMemoryBarriers.push_back ( { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr, s.get ().srcAccessMask, s.get ().dstAccessMask, 
				s.get ().srcQueueFamilyIndex, s.get ().dstQueueFamilyIndex, s.get ().buffer, s.get ().offset, s.get ().size } );

		return *this;
	}

	Barrier&	imageBarriers ( std::initializer_list<std::reference_wrapper<ImageMemory>> bl )
	{
		for ( auto& s : bl )
			imageMemoryBarriers.push_back ( { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, s.get ().srcAccessMask, s.get ().dstAccessMask, 
				s.get ().oldLayout, s.get ().newLayout,  s.get ().srcQueueFamilyIndex, s.get ().dstQueueFamilyIndex, s.get ().image, s.get ().subresourceRange } );

		return *this;
	}
};

class	CommandBuffer
{
	Device                * device        = nullptr;
	VkCommandBuffer			buffer        = VK_NULL_HANDLE;
	VkPipelineBindPoint		bindingPoint  = VK_PIPELINE_BIND_POINT_GRAPHICS;
	VkPipelineLayout		layout        = VK_NULL_HANDLE;
	bool					hasRenderPass = false;

public:
	CommandBuffer () = default;
	CommandBuffer ( CommandBuffer&& cb )
	{
		std::swap ( device,       cb.device );
		std::swap ( buffer,       cb.buffer );
		std::swap ( bindingPoint, cb.bindingPoint );
		std::swap ( layout,       cb.layout );
	}
	CommandBuffer ( const CommandBuffer& ) = delete;
	~CommandBuffer ()
	{
		clean ();
	}

	CommandBuffer& operator = ( const CommandBuffer& ) = delete;

	void	clean ()
	{
		device->freeCommandBuffer ( *this );

		buffer = VK_NULL_HANDLE;
	}

	VkCommandBuffer	getHandle ()
	{
		return buffer;
	}

	CommandBuffer&	create ( Device& dev );
	CommandBuffer&	begin ( bool simultenous = false );
	CommandBuffer&	end ();

	CommandBuffer&	pipeline ( GraphicsPipeline& p )
	{
		layout       = p.getLayout ();
		bindingPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		vkCmdBindPipeline ( buffer, bindingPoint, p.getHandle () );

		return *this;
	}

	CommandBuffer&	pipeline ( ComputePipeline& p )
	{
		layout       = p.getLayout ();
		bindingPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

		vkCmdBindPipeline ( buffer, bindingPoint, p.getHandle () );

		return *this;
	}

	CommandBuffer&	addDescriptorSets ( std::initializer_list<std::reference_wrapper<DescriptorSet>> descriptorList )
	{
		std::vector<VkDescriptorSet>	descriptors;

		for ( auto& d : descriptorList )
			descriptors.push_back ( d.get().getHandle () );

		vkCmdBindDescriptorSets ( buffer, bindingPoint, layout, 0, (uint32_t)descriptors.size (), descriptors.data (), 0, nullptr );

		return *this;
	}

		// add support of dynamic offsets
	CommandBuffer&	addDescriptorSets ( std::initializer_list<std::reference_wrapper<DescriptorSet>> descriptorList, std::initializer_list<uint32_t> offsetList )
	{
		std::vector<VkDescriptorSet>	descriptors;
		std::vector<uint32_t>			offsets;

		for ( auto& d : descriptorList )
			descriptors.push_back ( d.get().getHandle () );

		for ( auto offs : offsetList )
			offsets.push_back ( offs );

		vkCmdBindDescriptorSets ( buffer, bindingPoint, layout, 0, (uint32_t)descriptors.size (), descriptors.data (), (uint32_t) offsets.size (), offsets.data () );

		return *this;
	}

	CommandBuffer&	beginRenderPass ( RenderPassInfo& pass, bool contentInline = true )
	{
		hasRenderPass = true;

		vkCmdBeginRenderPass  ( buffer, &pass.getInfo (), contentInline ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS );

		return *this;
	}

	CommandBuffer&	bindVertexBuffers ( std::initializer_list<std::pair<std::reference_wrapper<Buffer>, VkDeviceSize>> buffers )
	{
		std::vector<VkBuffer>		bufs;
		std::vector<VkDeviceSize>	offsets;

		for ( auto& b : buffers )
		{
			bufs.push_back ( b.first.get().getHandle () );
			offsets.push_back ( b.second );
		}

		vkCmdBindVertexBuffers ( buffer, 0, (uint32_t)bufs.size (), bufs.data (), offsets.data () );

		return *this;
	}

	CommandBuffer&	bindIndexBuffer ( Buffer& indexBuffer, VkIndexType indexType = VK_INDEX_TYPE_UINT16 )
	{
		vkCmdBindIndexBuffer ( buffer, indexBuffer.getHandle (), 0, indexType );

		return *this;
	}

	CommandBuffer&	pushConstants ( VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offs, uint32_t size, const void * ptr )
	{
		vkCmdPushConstants ( buffer, layout, stageFlags, offs, size, ptr );

		return *this;
	}

	template<typename T>
	CommandBuffer&	pushConstants ( VkPipelineLayout layout, VkShaderStageFlags stageFlags, const T& consts, uint32_t offs = 0 )
	{
		return pushConstants ( layout, stageFlags, offs, sizeof ( T ), &consts );
	}

	CommandBuffer&	dispatch ( uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1 )
	{
		vkCmdDispatch ( buffer, groupCountX, groupCountY, groupCountZ );

		return *this;
	}

	CommandBuffer&	draw ( uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0 )
	{
		vkCmdDraw ( buffer, vertexCount, instanceCount, firstVertex, firstInstance );

		return *this;
	}

	CommandBuffer&	drawIndexed ( uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance  = 0 )
	{
		vkCmdDrawIndexed ( buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );

		return *this;
	}

	template <class C>
	CommandBuffer& render ( C * obj )
	{
		obj->render ( *this );				// assume C has method void render ( CommandBuffer& )

		return *this;
	}

	template <class C>
	CommandBuffer& renderInstanced ( C * obj, uint32_t numInstances )
	{
		obj->render ( *this, numInstances );		// assume C has method void render ( CommandBuffer& )

		return *this;
	}

	CommandBuffer&	resetEvent ( Event& event, VkPipelineStageFlags flags )
	{
		vkCmdResetEvent ( buffer, event.getHandle (), flags );

		return *this;
	}

	
	CommandBuffer&	setEvent ( Event& event, VkPipelineStageFlags flags )
	{
		vkCmdSetEvent ( buffer, event.getHandle (), flags );

		return *this;
	}

	CommandBuffer&	waitEvents (std::initializer_list<std::reference_wrapper<Event>> eventList, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask )
	{
		std::vector<VkEvent>	events;

		for ( auto& d : eventList )
			events.push_back ( d.get().getHandle () );

		vkCmdWaitEvents ( buffer, (uint32_t) events.size (), events.data (), srcStageMask, dstStageMask, 0, nullptr, 0, nullptr, 0, nullptr );

		return *this;
	}

	CommandBuffer&	barrier ( Barrier& b )
	{
		vkCmdPipelineBarrier ( buffer, b.srcStageMask (), b.dstStageMask (), b.dependencyFlags (), (uint32_t)b.memoryBarriers.size (), b.memoryBarriers.data (), 0, nullptr, (uint32_t)b.imageMemoryBarriers.size (), b.imageMemoryBarriers.data () );
		
		return *this;
	}
};

class	SubmitInfo
{
	VkSubmitInfo						submitInfo = {};
	std::vector<VkSemaphore>			waitSemaphores;
	std::vector<VkSemaphore>			signalSemaphores;
	std::vector<VkPipelineStageFlags>	stageFlags;
	std::vector<VkCommandBuffer>		commandBuffers;

public:
	SubmitInfo () = default;
	SubmitInfo ( const SubmitInfo& ) = delete;

	SubmitInfo& operator = ( const SubmitInfo& ) = delete;

	SubmitInfo& wait ( std::initializer_list<std::pair<VkSemaphore, VkPipelineStageFlags>> waitList )
	{
		for ( auto& s : waitList )
		{
			waitSemaphores.push_back ( s.first );
			stageFlags.push_back ( s.second );
		}

		return *this;
	}

	SubmitInfo& wait ( std::initializer_list<std::pair<std::reference_wrapper<Semaphore>, VkPipelineStageFlags>> waitList )
	{
		for ( auto& s : waitList )
		{
			waitSemaphores.push_back ( s.first.get().getHandle () );
			stageFlags.push_back ( s.second );
		}

		return *this;
	}

	SubmitInfo& buffers ( std::initializer_list<std::reference_wrapper<CommandBuffer>> buffers )
	{
		for ( auto& b : buffers )
			commandBuffers.push_back ( b.get ().getHandle () );

		return *this;
	}

	SubmitInfo& signal ( std::initializer_list<VkSemaphore> semaphores )
	{
		for ( auto& s : semaphores )
			signalSemaphores.push_back ( s );

		return *this;
	}

	SubmitInfo& signal ( std::initializer_list<std::reference_wrapper<Semaphore>> semaphores )
	{
		for ( auto& s : semaphores )
			signalSemaphores.push_back ( s.get ().getHandle () );

		return *this;
	}

	bool	submit ( VkQueue queue, VkFence fence = VK_NULL_HANDLE );
};

VkImageMemoryBarrier2  imageBarrier  ( VkImage image, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, uint32_t baseMipLevel = 0, uint32_t levelCount = VK_REMAINING_MIP_LEVELS );
VkBufferMemoryBarrier2 bufferBarrier ( VkBuffer buffer, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask );

inline VkImageMemoryBarrier2  imageBarrier  ( Image& image, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, uint32_t baseMipLevel = 0, uint32_t levelCount = VK_REMAINING_MIP_LEVELS )
{
	return imageBarrier  ( image.getHandle (), srcStageMask, srcAccessMask, oldLayout, dstStageMask, dstAccessMask, newLayout, aspectMask, baseMipLevel, levelCount );
}

inline VkBufferMemoryBarrier2 bufferBarrier ( Buffer& buffer, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask )
{
	return bufferBarrier ( buffer.getHandle (), srcStageMask, srcAccessMask, dstStageMask, dstAccessMask );
}

void pipelineBarrier ( CommandBuffer& cb, std::initializer_list<VkBufferMemoryBarrier2> bufBerriers, VkDependencyFlags dependencyFlags = 0 );
void pipelineBarrier ( CommandBuffer& cb, std::initializer_list<VkImageMemoryBarrier2> imgBarriers, VkDependencyFlags dependencyFlags = 0);
void pipelineBarrier ( CommandBuffer& cb, std::initializer_list<VkBufferMemoryBarrier2> bufBerriers, std::initializer_list<VkBufferMemoryBarrier2>& imgBarriers, VkDependencyFlags dependencyFlags = 0 );

