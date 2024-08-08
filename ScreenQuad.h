//
// Simple class to render screen quad
// Params are passed to vertex shader as a vec4 args ( width, height, sMax, tmax )
//

#pragma once

#include	"Buffer.h"

class	ScreenQuad
{
	Buffer	buffer;
	
public:
	ScreenQuad () = default;
/*
	ScreenQuad ( ScreenQuad&& q )
	{
		std::swap ( buffer, q.buffer );
	}
*/
	ScreenQuad ( const ScreenQuad& ) = delete;	

	ScreenQuad& operator = ( const ScreenQuad& ) = delete;

	bool	isOk () const
	{
		return buffer.isOk ();
	}

	CommandBuffer&	render ( CommandBuffer& commandBuffer ) 
	{
		assert ( buffer.getHandle () != VK_NULL_HANDLE );
/*
		VkBuffer		vertexBuffers [] = { buffer.getHandle () };
		VkDeviceSize	offsets       [] = { 0 };

		vkCmdBindVertexBuffers ( commandBuffer, 0, 1, vertexBuffers, offsets );
		vkCmdDraw              ( commandBuffer, 6, 1, 0, 0 );
*/
		return commandBuffer.bindVertexBuffers ( { {buffer, 0} } ).draw ( 6 );
	}

			// register vertex attributes in pipeline
	GraphicsPipeline&	setVertexAttrs ( GraphicsPipeline& pipeline )
	{
		pipeline.addVertexAttr ( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 );
				
		return pipeline;
	}
			// create buffer
	void	create ( Device& device )
	{
		static const float vertices [] = 
		{
			-1, -1, 0, 0,
			-1,  1, 0, 1,
			 1,  1, 1, 1,

			-1, -1, 0, 0,
			 1,  1, 1, 1,
			 1, -1, 1, 0
		};

		uint32_t			size = sizeof ( vertices );
		Buffer				stagingBuffer;
		SingleTimeCommand	cmd ( device );

					// use staging buffer to copy data to GPU-local memory
		stagingBuffer.create ( device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		stagingBuffer.copy   ( vertices, size );
		buffer.create        ( device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		buffer.copyBuffer    ( cmd, stagingBuffer, size );
	}
};
