#pragma once
#include	"CommandBuffer.h"
#include	"Device.h"

class	StatisticsPool
{
	Device				          * device     = nullptr;
	VkQueryPool						queryPool  = VK_NULL_HANDLE;
	uint32_t						count      = 0;
	VkQueryPipelineStatisticFlags	statistics = 0;

	StatisticsPool () = default;
	~StatisticsPool ()
	{
		destroy ();
	}

		// create pool with cnt slots
	bool	create ( Device& dev, VkQueryPipelineStatisticFlags stats, uint32_t cnt )
	{
		VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };

		device                        = &dev;
		count                         = cnt;
		statistics                    = stats;
		createInfo.queryType          = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		createInfo.queryCount         = count;
		createInfo.pipelineStatistics = stats;

		return vkCreateQueryPool ( device->getDevice (), &createInfo, nullptr, &queryPool ) == VK_SUCCESS;
	}

	void	destroy ()
	{
		if ( queryPool && device )
			vkDestroyQueryPool ( device->getDevice (), queryPool, nullptr );
	}

		// reset all timestamps in the pool
	void	reset ( CommandBuffer& commandBuffer, uint32_t first = 0, uint32_t num = 0 )
	{
		if ( num < 1 )
			num = count;

		vkCmdResetQueryPool ( commandBuffer.getHandle (), queryPool, first, num );
	}

	void	begin ( CommandBuffer& commandBuffer, uint32_t index )
	{
		vkCmdBeginQuery ( commandBuffer.getHandle (), queryPool, index, 0 );
	}

	void	end ( CommandBuffer& commandBuffer, uint32_t index )
	{
		vkCmdEndQuery ( commandBuffer.getHandle (), queryPool, index );
	}

		// get selected timestamps from GPU
	std::vector<uint64_t>	getResults ( uint32_t first = 0, uint32_t num = 0 )
	{
		if ( num == 0 )
			num = count;

		std::vector<uint64_t>	results ( num );

		if (  vkGetQueryPoolResults ( device->getDevice (), queryPool, first, num, uint32_t (results.size ()) * sizeof(uint64_t), results.data (), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT ) == VK_SUCCESS )
			return results;

		return {};
	}

	int	getNumResults () const
	{
		int	sum = 0;

		for ( int i = 0; i < sizeof ( statistics ) * 8; i++ )
			if ( statistics & (1 << i) )
				sum++;

		return sum;
	}

	int	indexForFlag ( VkQueryPipelineStatisticFlags bit )
	{
		int	pos = 0;

		for ( int i = 0; (1u << i) < bit; i++ )
			if ( statistics & (1 << i) )
				pos++;

		return pos;
	}
};
