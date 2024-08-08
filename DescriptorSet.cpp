#include	"DescriptorSet.h"
#include	"Pipeline.h"

static VkDescriptorPool createPool ( VkDevice device, const DescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags = 0 )
{
    std::vector<VkDescriptorPoolSize>	sizes;
    VkDescriptorPoolCreateInfo          pool_info = {};
    VkDescriptorPool                    descriptorPool;
    
    sizes.reserve ( poolSizes.sizes.size() );
    
    for ( auto sz : poolSizes.sizes )
        sizes.push_back ( { sz.first, uint32_t(sz.second * count) } );
    
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = flags;
    pool_info.maxSets       = count;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes    = sizes.data();

    vkCreateDescriptorPool ( device, &pool_info, nullptr, &descriptorPool );

    return descriptorPool;
}

void DescriptorAllocator::create ( Device& newDevice )
{
    device = newDevice.getDevice ();
}

void DescriptorAllocator::clean ()
{
        // delete every pool held
    for ( auto p : freePools )
        vkDestroyDescriptorPool ( device, p, nullptr );
    
    for ( auto p : usedPools )
        vkDestroyDescriptorPool ( device, p, nullptr );
    
    freePools.clear ();
    usedPools.clear ();

	currentPool = VK_NULL_HANDLE;
}

VkDescriptorPool DescriptorAllocator::pickPool ()
{    
    if ( freePools.size () > 0 )	// there are reusable pools available
    {
            // grab pool from the back of the vector and remove it from there.
        VkDescriptorPool pool = freePools.back ();
        freePools.pop_back();
        
        return pool;
    }
    else
            // no pools available, so create a new one
        return createPool ( device, descriptorSizes, 1000, flags );
}

VkDescriptorSet	DescriptorAllocator::alloc ( VkDescriptorSetLayout layout )
{
        // initialize the currentPool handle if it's null
    if ( currentPool == VK_NULL_HANDLE )
    {
        currentPool = pickPool ();
        usedPools.push_back ( currentPool );
    }

	VkDescriptorSet				set       = VK_NULL_HANDLE;
	VkDescriptorSetAllocateInfo allocInfo = {};
    
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext              = nullptr;
    allocInfo.pSetLayouts        = &layout;
    allocInfo.descriptorPool     = currentPool;
    allocInfo.descriptorSetCount = 1;

        // try to allocate the descriptor set
    VkResult allocResult    = vkAllocateDescriptorSets ( device, &allocInfo, &set );
    bool     needReallocate = false;

    if ( allocResult == VK_SUCCESS )
        return set;
    
        // some strange error
    if ( allocResult != VK_ERROR_FRAGMENTED_POOL && allocResult != VK_ERROR_OUT_OF_POOL_MEMORY )
        return VK_NULL_HANDLE;
    
        // allocate a new pool and retry
    currentPool              = pickPool ();
    allocInfo.descriptorPool = currentPool;        // ????
    usedPools.push_back ( currentPool );

    allocResult = vkAllocateDescriptorSets ( device, &allocInfo, &set );

            // if it still fails then we have big issues
    return allocResult == VK_SUCCESS ? set : VK_NULL_HANDLE;
}

void DescriptorAllocator::reset ()
{
        // reset all used pools and add them to the free pools
    for ( auto p : usedPools )
    {
        vkResetDescriptorPool ( device, p, 0 );
        freePools.push_back ( p );
    }

        // clear the used pools, since we've put them all in the free pools
    usedPools.clear ();

        // reset the current pool handle back to null
    currentPool = VK_NULL_HANDLE;
}

DescriptorSet&	DescriptorSet::setLayout (  Device& dev, DescriptorAllocator& descAllocator, const DescSetLayout& descSetLayout )
{
	device              = &dev;
	descriptorSetLayout = descSetLayout.getHandle ();
	allocator           = &descAllocator;

	return *this;
}
