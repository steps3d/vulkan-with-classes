#pragma once

#ifdef _WIN32
#include	<Windows.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include	"Device.h"
#include	"Log.h"
#include	"SingleTimeCommand.h"

class GpuMemory
{
	VkDeviceSize	size   = 0;
	Device        * device = nullptr;
	VkDeviceMemory	memory = VK_NULL_HANDLE;

public:
	GpuMemory () {}
	GpuMemory ( GpuMemory&& m )
	{
		std::swap ( memory, m.memory );
	}
	GpuMemory ( const GpuMemory& ) = delete;
	GpuMemory ( Device& dev, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags properties )
	{
		alloc ( dev, memRequirements, properties );
	}

	~GpuMemory () 
	{
		clean ();
	}

	GpuMemory& operator = ( const GpuMemory& ) = delete;

	bool	isOk () const
	{
		return device != nullptr && size != 0 && memory != VK_NULL_HANDLE;
	}

	VkDevice	getDevice () const
	{
		return device->getDevice ();
	}

	VkPhysicalDevice	getPhysicalDevice () const
	{
		return device->getPhysicalDevice ();
	}

	VkDeviceMemory	getMemory () const
	{
		return memory;
	}

	VkDeviceSize	getSize () const
	{
		return size;
	}

	void	clean ()
	{
		if ( memory != VK_NULL_HANDLE )
			vkFreeMemory ( device->getDevice (), memory, nullptr );

		memory = VK_NULL_HANDLE;
	}

	bool	alloc ( Device& _device, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags properties )
	{
		VkMemoryAllocateInfo allocInfo = {};
		
		device                    = &_device;
		size                      = memRequirements.size;
		allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize  = size;
		allocInfo.memoryTypeIndex = findMemoryType ( memRequirements.memoryTypeBits, properties );

		return vkAllocateMemory ( device->getDevice (), &allocInfo, nullptr, &memory ) == VK_SUCCESS;
	}

	uint32_t findMemoryType ( uint32_t typeFilter, VkMemoryPropertyFlags properties ) const
	{
		auto&	memProperties = device->getMemoryProperties ();
		
		for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
			if ( (typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties ) 
				return i;

		fatal () << "GpuMemory: failed to find suitable memory type! " << properties;

		return 0;		// we won't get here, but compiler complains about returning no value
	}

	bool	copy ( const void * ptr, VkDeviceSize size, size_t offs = 0 )
	{
		void * data;

		if ( !memory )
			return false;

		vkMapMemory   ( device->getDevice (), memory, 0, size, 0, &data );
		memcpy        ( offs + (char *) data,  ptr, size );
		vkUnmapMemory ( device->getDevice (), memory );

		// vkInvalidateMappedMemoryRanges if not host coherent
		return true;
	}

	void * map ( VkDeviceSize offs = 0, VkDeviceSize size = VK_WHOLE_SIZE )
	{
		void * data;

		if ( !memory )
			return false;

		if ( vkMapMemory ( device->getDevice (), memory, 0, size, 0, &data ) != VK_SUCCESS )
			return nullptr;

		return data;
	}

	void	unmap ()
	{
		vkUnmapMemory ( device->getDevice (), memory );
	}
};

class Buffer
{
protected:
	VkBuffer	buffer  = VK_NULL_HANDLE;
	int			mapping = 0;

#ifdef USE_VMA
	Device		  * device     = nullptr;
	VmaAllocation	allocation = VK_NULL_HANDLE;
#else
	GpuMemory	memory;
#endif // USE_VMA

public:
	enum
	{
		hostRead  = 1,
		hostWrite = 2
	};

	Buffer () {}
#ifdef USE_VMA
	Buffer ( Buffer&& b )
	{
		std::swap ( allocation, b.allocation  );
		std::swap ( buffer, b.buffer );
		std::swap ( mapping, b.mapping );
}
#else
	Buffer ( Buffer&& b ) : memory ( std::move ( b.memory ) )
	{
		std::swap ( buffer, b.buffer );
		std::swap ( mapping, b.mapping );
	}
#endif // USE_VMA

	Buffer ( const Buffer& ) = delete;
	Buffer ( Device& dev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties )
	{
		create ( dev, size, usage, properties );
	}

	~Buffer () 
	{
		clean ();
	}

	Buffer& operator = ( const Buffer& ) = delete;

	bool	isOk () const
	{
#ifdef USE_VMA
		return allocation != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE;
#else
		return memory.isOk () && buffer != VK_NULL_HANDLE;
#endif // USE_VMA
	}

	VkDevice	getDevice () const
	{
#ifdef USE_VMA
		return device->getDevice ();
#else
		return memory.getDevice ();
#endif // USE_VMA
	}

#ifndef USE_VMA
	GpuMemory&	getMemory ()
	{
		return memory;
	}
#endif // !USE_VMA

	VkBuffer	getHandle () const
	{
		return buffer;
	}
	
	void	clean ()
	{
#ifdef USE_VMA
		if ( buffer != VK_NULL_HANDLE )
			vmaDestroyBuffer ( device->getAllocator (), buffer, allocation );

		allocation = VK_NULL_HANDLE;
#else
		if ( buffer != VK_NULL_HANDLE )
			vkDestroyBuffer ( memory.getDevice (), buffer, nullptr );

		memory.clean ();
#endif // USE_VMA

		buffer = VK_NULL_HANDLE;
	}

	bool	create ( Device& dev, VkDeviceSize size, VkBufferUsageFlags usage, int mappable )	//VkMemoryPropertyFlags properties )
	{
		VkBufferCreateInfo		bufferInfo = {};

		bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size        = size;
		bufferInfo.usage       = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

#ifdef USE_VMA
		VmaAllocationCreateInfo	allocInfo = {};

		device          = &dev;
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		if ( mappable & hostRead )
			allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

		if ( mappable & hostWrite )
			allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

		if ( vmaCreateBuffer ( device->getAllocator (), &bufferInfo, &allocInfo, &buffer, &allocation, nullptr ) != VK_SUCCESS )
			fatal () << "Buffer: failed to create buffer" << std::endl;
#else
		VkMemoryRequirements	memRequirements;
		VkMemoryPropertyFlags	properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ;

		if ( mappable )
			properties |=  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		if ( vkCreateBuffer ( device.getDevice (), &bufferInfo, nullptr, &buffer ) != VK_SUCCESS )
			fatal () << "Buffer: failed to create buffer!";

		vkGetBufferMemoryRequirements ( device.getDevice (), buffer, &memRequirements );

		if ( !memory.alloc ( dev, memRequirements, properties ) )
			fatal () << "Buffer: cannot allocate memorry";

		vkBindBufferMemory ( device.getDevice (), buffer, memory.getMemory (), 0 );
#endif // USE_VMA

		return true;
	}

	template <typename T>
	bool	create ( Device& device, VkBufferUsageFlags usage, const std::vector<T>& data, int mappable )
	{
		if ( !create ( device, data.size () * sizeof ( T ), usage, mappable ) )
			return false;

		return copy ( data );
	}

	bool	copy ( const void * ptr, VkDeviceSize size, size_t offs = 0 )
	{
#ifdef USE_VMA
		void * bufPtr;

		if ( vmaMapMemory ( device->getAllocator (), allocation, &bufPtr ) != VK_SUCCESS )
			return false;

		memcpy ( bufPtr, ptr, size );

		vmaUnmapMemory ( device->getAllocator (), allocation );

		return true;
#else
		return memory.copy ( ptr, size, offs );
#endif // USE_VMA
	}

	template <typename T>
	bool	copy ( const std::vector<T>& data )
	{
		return copy ( data.data (), data.size () * sizeof ( T ) );
	}

	template <typename T>
	bool	copy ( const T& data )
	{
		return copy ( &data, sizeof ( T ) );
	}

	void	copyBuffer ( SingleTimeCommand& cmd, Buffer& fromBuffer, VkDeviceSize size )
	{
		VkBufferCopy	copyRegion    = {};

		copyRegion.size = size;

		vkCmdCopyBuffer ( cmd.getHandle (), fromBuffer.getHandle (), getHandle (), 1, &copyRegion );
	}
};

class	PersistentBuffer : public Buffer
{
	void  * ptr = nullptr;		// pointer to actual data

public:
	PersistentBuffer () : Buffer () {}
	PersistentBuffer ( const PersistentBuffer& ) = delete;
	PersistentBuffer ( PersistentBuffer&& buf ) : Buffer ( std::move ( buf ) )
	{
		std::swap ( ptr, buf.ptr );
	}
	~PersistentBuffer ()
	{
		clean ();
	}

	void	clean ()
	{
#ifdef USE_VMA
		vmaUnmapMemory ( device->getAllocator (), allocation );
#else
		getMemory ().unmap ();
#endif // USE_VMA

		Buffer::clean ();

		ptr = nullptr;
	}

	bool	create ( Device& dev, VkDeviceSize size, VkBufferUsageFlags usage, int mappable )
	{
		if ( !Buffer::create ( dev, size, usage, mappable ) )
			return false;

#ifdef USE_VMA
		if ( vmaMapMemory ( device->getAllocator (), allocation, &ptr ) != VK_SUCCESS )
			return false;
#else
		ptr = getMemory().map ( 0 );
#endif // USE_VMA

		return ptr != nullptr;
	}

	void * getPtr () const
	{
		return ptr;
	}
};

template<typename T>
class	Uniform : public PersistentBuffer
{
	int	count    = 0;					// count * T 
	int	itemSize = sizeof ( T );		// may be different due to aligning
	int	align    = 1;

public:
	Uniform () : PersistentBuffer () {}

	T * getPtr () const
	{
		return (T *) PersistentBuffer::getPtr ();
	}

	bool	create ( Device& device, VkBufferUsageFlags usage, int n = 1, int al = 1 )
	{
		align    = al;
		count    = n;
		itemSize = ( ( sizeof ( T ) + align - 1 ) / align ) * align;

		return PersistentBuffer::create ( device, count * itemSize, usage, Buffer::hostWrite );
	}

	T * operator -> () const
	{
		return getPtr ();
	}

	uint32_t	size () const
	{
		return itemSize;
	}

	uint32_t	offsForItem ( int i ) const
	{
		return i * itemSize;
	}

	T&	operator [] ( int i )
	{
		assert ( i >= 0 && i < count );

		return *(T *)(i*itemSize + (char *)getPtr ());
	}
};