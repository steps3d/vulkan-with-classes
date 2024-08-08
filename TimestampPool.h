#pragma once
#include	"Device.h"


class	TimestampPool
{
	Device        * device     = nullptr;
	VkQueryPool	queryPpool = VK_NULL_HANDLE;
	uint32_t	count      = 0;

	TimestampPool () = default;
	~TimestampPool ()
	{
		destroy ();
	}

		// create pool with cnt slots
	bool	create ( Device& dev, uint32_t cnt = 128 )
	{
		VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };

		device                = &dev;
		count                 = cnt;
		createInfo.queryType  = VK_QUERY_TYPE_TIMESTAMP;
		createInfo.queryCount = count;

		return vkCreateQueryPool ( device->getDevice (), &createInfo, nullptr, &queryPool ) == VK_SUCCESS;
	}

	void	destroy ()
	{
		if ( queryPool && device )
			vkDestroyQueryPool ( device.getDevice (), queryPool, nullptr );
	}

		// reset all timestamps in the pool
	void	reset ( CommandBuffer& commandBuffer, uint32_t first = 0, uint32_t num = 0 )
	{
		if ( num < 1 )
			num = count;

		vkCmdResetQueryPool ( commandBuffer.getHandle (), queryPool, first, num );
	}

		// write timestamp with given no
	void	writeTimestamp ( CommandBuffer& commandBuffer, int no, VkPipelineStageFlagBits stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT )
	{
		vkCmdWriteTimestamp ( commandBuffer.getHandle (), stageMask, queryPool, no );
	}

		// get selected timestamps from GPU
	bool	getResults ( std::vector<uint64_t>& results, uint32_t first = 0, uint32_t num = 0 )
	{
		if ( num == 0 )
			num = count;

		if ( num > results.size () )
			num = uint32_t ( results.size () );

		return vkGetQueryPoolResults ( device->getDevice (), queryPool, first, num, uint32_t (results.size ()) * sizeof(uint64_t), results.data (), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT ) == VK_SUCCESS;
	}

		// convert  timestamp value to milliseconds
	double	convertToMs ( uint64_t value )
	{
		return double(value) * device->properties.limits.timestampPeriod * 1e-6;
	}
};
