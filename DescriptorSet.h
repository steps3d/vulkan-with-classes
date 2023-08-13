#pragma once

#include	<assert.h>
#include	<vector>
#include	"Buffer.h"
#include	"Texture.h"

class	DescSetLayout;

class DescriptorAllocator
{
public:
    struct PoolSizes
    {
        std::vector<std::pair<VkDescriptorType,float>> sizes =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER,                0.5f },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f  },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          4.f  },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1.f  },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1.f  },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1.f  },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         2.f  },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         2.f  },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f  },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f  },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       0.5f }
        };
    };

	void create ( Device& newDevice );	// start allocator
	void reset ();                      // reset all pools and move them to freePools
	void clean ();                      // destroy allocator
                                        // allocate descriptor set
	VkDescriptorSet	alloc ( VkDescriptorSetLayout layout );

	void	setMultiplier ( VkDescriptorType type, float factor )
	{
		for ( auto& p : descriptorSizes.sizes )
			if ( p.first == type )
				p.second = factor;
	}

private:
	VkDescriptorPool pickPool ();		// pick appropriate pool for allocation

	VkDevice						device      = VK_NULL_HANDLE;
	VkDescriptorPool                currentPool = VK_NULL_HANDLE;
	PoolSizes                       descriptorSizes;
	std::vector<VkDescriptorPool>	usedPools;        // active pools with allocated items
	std::vector<VkDescriptorPool>	freePools;  
};

class	DescriptorSet
{
	Device							  * device              = nullptr;
	DescriptorAllocator			      * allocator           = nullptr;
	VkDescriptorSet						set                 = VK_NULL_HANDLE;
	VkDescriptorSetLayout				descriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkWriteDescriptorSet>	writes;

public:
	DescriptorSet () = default;
	DescriptorSet ( DescriptorSet&& ) 
	{ 
		fatal () << "DescriptorSet move c-tor called" << Log::endl; 
	}
	DescriptorSet ( const DescriptorSet& ) = delete;
	~DescriptorSet ()
	{
		clean ();
	}

	VkDescriptorSet	getHandle () const
	{
		return set;
	}

	void	clean ()
	{
		for ( auto& d : writes )
		{
			delete d.pBufferInfo;
			delete d.pImageInfo;
		}

		writes.clear ();
	}

	DescriptorSet&	setLayout (  Device& dev, DescriptorAllocator& descAllocator, const DescSetLayout& descSetLayout );

	DescriptorSet&	addBuffer ( uint32_t binding, VkDescriptorType type, Buffer& buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE )
	{
		if ( set == VK_NULL_HANDLE )
			alloc ();
		
		VkDescriptorBufferInfo * bufferInfo       = new VkDescriptorBufferInfo {};
		VkWriteDescriptorSet	 descriptorWrites = {};

		bufferInfo->buffer = buffer.getHandle ();
		bufferInfo->offset = offset;
		bufferInfo->range  = size;

		descriptorWrites.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites.dstSet          = set;
		descriptorWrites.dstBinding      = binding;
		descriptorWrites.dstArrayElement = 0;
		descriptorWrites.descriptorType  = type;
		descriptorWrites.descriptorCount = 1;
		descriptorWrites.pBufferInfo     = bufferInfo;

		writes.push_back ( descriptorWrites );

		return *this;
	}

	DescriptorSet& addUniformBuffer ( uint32_t binding, Buffer& buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE )
	{
		return addBuffer ( binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer, offset, size );
	}

	DescriptorSet& addStorageBuffer ( uint32_t binding, Buffer& buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE )
	{
		return addBuffer ( binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer, offset, size );
	}

	DescriptorSet&	addImage ( uint32_t binding, Texture& texture, Sampler& sampler )
	{
		if ( set == VK_NULL_HANDLE )
			alloc ();
		
		assert ( texture.getImageView () != VK_NULL_HANDLE );
		assert ( sampler.getHandle    () != VK_NULL_HANDLE );

		VkDescriptorImageInfo    * imageInfo        = new VkDescriptorImageInfo {};
		VkWriteDescriptorSet	   descriptorWrites = {};

		imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo->imageView   = texture.getImageView ();
		imageInfo->sampler     = sampler.getHandle    ();

		descriptorWrites.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites.dstSet          = set;
		descriptorWrites.dstBinding      = binding;
		descriptorWrites.dstArrayElement = 0;
		descriptorWrites.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites.descriptorCount = 1;
		descriptorWrites.pImageInfo      = imageInfo;

		writes.push_back ( descriptorWrites );

		return *this;
	}

	DescriptorSet&	addImageArray ( uint32_t binding, std::initializer_list<std::reference_wrapper<Texture>> textureList )
	{
		assert ( textureList.size () > 0 );

		if ( set == VK_NULL_HANDLE )
			alloc ();

		std::vector<VkDescriptorImageInfo>	views;

		for ( auto& tx : textureList )
		{
			VkDescriptorImageInfo	info;

			info.sampler     = nullptr;
			info.imageView   = tx.get().getImageView ();
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			views.push_back ( info );
		}

		VkWriteDescriptorSet	   descriptorWrites = {};

		descriptorWrites.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites.dstSet          = set;
		descriptorWrites.dstBinding      = binding;
		descriptorWrites.dstArrayElement = 0;
		descriptorWrites.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		descriptorWrites.descriptorCount = (uint32_t)views.size ();
		descriptorWrites.pImageInfo      = views.data ();

		writes.push_back ( descriptorWrites );

		return *this;
		return *this;
	}

	DescriptorSet&	addImageArray ( uint32_t binding, const std::vector<Texture>& textures )
	{
		assert ( textures.size () > 0 );

		if ( set == VK_NULL_HANDLE )
			alloc ();
		
		VkDescriptorImageInfo *	views = new VkDescriptorImageInfo [textures.size ()];
		VkDescriptorImageInfo *	v     = views;

		for ( auto& tx : textures )
		{
			v->sampler     = nullptr;
			v->imageView   = tx.getImageView ();
			v->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			v++;
		}

		VkWriteDescriptorSet	   descriptorWrites = {};

		descriptorWrites.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites.dstSet          = set;
		descriptorWrites.dstBinding      = binding;
		descriptorWrites.dstArrayElement = 0;
		descriptorWrites.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		descriptorWrites.descriptorCount = (uint32_t)textures.size ();
		descriptorWrites.pImageInfo      = views;

		writes.push_back ( descriptorWrites );

		return *this;
	}

	DescriptorSet&	addSampler ( uint32_t binding, Sampler& sampler )
	{
		VkWriteDescriptorSet	descriptorWrites = {};
		VkDescriptorImageInfo * samplerInfo      = new VkDescriptorImageInfo;

		if ( set == VK_NULL_HANDLE )
			alloc ();

		*samplerInfo = {};
		samplerInfo->sampler             = sampler.getHandle ();
		descriptorWrites.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites.dstSet          = set;
		descriptorWrites.dstBinding      = binding;
		descriptorWrites.dstArrayElement = 0;
		descriptorWrites.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
		descriptorWrites.descriptorCount = 1;
		descriptorWrites.pBufferInfo     = 0;
		descriptorWrites.pImageInfo      = samplerInfo;

		writes.push_back ( descriptorWrites );

		return *this;
	}
	
	void	create ()
	{
		if ( set == VK_NULL_HANDLE )
			alloc ();
				
		vkUpdateDescriptorSets ( device->getDevice (), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr );
	}

private:
	void	alloc ()
	{
		assert ( device != nullptr && allocator != nullptr && descriptorSetLayout != VK_NULL_HANDLE );

		set = allocator->alloc ( descriptorSetLayout);
	}
};
