//
// Class to hold simple meshes.
// For every vertex position, texture coordinate and TBN basis is stored
//

#pragma once
#ifndef	__BASIC_MESH__
#define	__BASIC_MESH__

#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SWIZZLE

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/mat4x4.hpp>

#include "Buffer.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Device.h"
#include "CommandBuffer.h"
#include "bbox.h"

struct  BasicVertex
{
	glm::vec3	pos;
	glm::vec2	tex;
	glm::vec3	n;
	glm::vec3	t, b;

	BasicVertex () = default;
	BasicVertex ( const glm::vec3& p, const glm::vec2& t ) : pos ( p ), tex ( t ) {}
};

template <>
inline GraphicsPipeline&	registerVertexAttrs<BasicVertex> ( GraphicsPipeline& pipeline ) 
{
	return pipeline
		.addVertexAttr ( 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(BasicVertex, pos) )		// binding, location, format, offset
		.addVertexAttr ( 0, 1, VK_FORMAT_R32G32_SFLOAT,    offsetof(BasicVertex, tex) )
		.addVertexAttr ( 0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(BasicVertex, n) )
		.addVertexAttr ( 0, 3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(BasicVertex, t) )
		.addVertexAttr ( 0, 4, VK_FORMAT_R32G32B32_SFLOAT, offsetof(BasicVertex, b) );
}

class Mesh
{
	Device		  * device = nullptr;
	Buffer			vertices;		// vertex data
	Buffer			indices;		// index buffer
	int	         	numVertices;
	int	         	numTriangles;
	std::string  	name;
	bbox		 	box;
	int			 	material;
	
public:
	Mesh ( Device& dev, BasicVertex * vertices, const uint32_t * indices, size_t nv, size_t nt );
	
	CommandBuffer&	render ( CommandBuffer& commandBuffer, uint32_t instanceCount = 1 )
	{
		return commandBuffer.bindVertexBuffers ( { { vertices, 0 } } ).bindIndexBuffer ( indices, VK_INDEX_TYPE_UINT32 ).drawIndexed ( numTriangles * 3, instanceCount );
	}

	void	render ( VkCommandBuffer commandBuffer )
	{
		VkBuffer		vertexBuffers [] = { vertices.getHandle () };
		VkDeviceSize	offsets       [] = { 0 };

		vkCmdBindVertexBuffers ( commandBuffer, 0, 1, vertexBuffers, offsets );
		vkCmdBindIndexBuffer   ( commandBuffer, indices.getHandle (), 0, VK_INDEX_TYPE_UINT32 );
		vkCmdDrawIndexed       ( commandBuffer, numTriangles*3, 1, 0, 0, 0 );
	}

	const std::string& getName () const
	{
		return name;
	}
	
	const bbox&	getBox () const
	{
		return box;
	}

	int getNumVertices () const
	{
		return numVertices;
	}
	
	int getNumTriangles () const
	{
		return numTriangles;
	}
	
	void setName ( const std::string& theName )
	{
		name = theName;
	}
	
	void setMaterial ( int m )
	{
		material = m;
	}
	
	int getMaterial () const
	{
		return material;
	}
	
private:
	void	createBuffer ( Buffer& buffer, uint32_t usage, size_t size, const void * data );	
};

class	BasicMaterial 
{
	std::string	name;
	std::string	diffMap;
	std::string	specMap;
	std::string	bumpMap;
	Texture	    tex;			// we need albedo, bump, metallness, roughness

public:
	BasicMaterial  () = default;
	~BasicMaterial () = default;

	void setName ( const std::string& s )
	{
		name = s;
	}

	const std::string& getName () const
	{
		return name;
	}

	void setDiffuseMap ( const std::string& n )
	{
		diffMap = n;
	}

	const std::string& getDiffuseMap () const
	{
		return diffMap;
	}
	
	void setSpecMap ( const std::string& n )
	{
		specMap = n;
	}

	void setBumpMap ( const std::string& n )
	{
		bumpMap = n;
	}

	const std::string& getBumpMap () const
	{
		return bumpMap;
	}
	
	bool load ( Device& device )
	{
		if ( diffMap.empty () )
			return true;
		
		tex.load ( device, diffMap );

		return true;
	}
	
	Texture& getTexture ()
	{
		return tex;
	}
};

extern const float pi;

void	computeTangents ( BasicVertex& v0, const BasicVertex& v1, const BasicVertex& v2 );
void	computeNormals  ( BasicVertex * vertices, const uint32_t * indices, size_t nv, size_t nt );

Mesh * createSphere  ( Device& dev, const glm::vec3& org, float radius, int n1, int n2 );
Mesh * createQuad    ( Device& dev, const glm::vec3& org, const glm::vec3& dir1, const glm::vec3& dir2 );
Mesh * createHorQuad ( Device& dev, const glm::vec3& org, float s1, float s2 );
Mesh * createBox     ( Device& dev, const glm::vec3& pos, const glm::vec3& size, const glm::mat4 * mat = nullptr, bool invertNormal = false );
Mesh * createTorus   ( Device& dev, float r1, float r2, int n1, int n2 );
Mesh * createKnot    ( Device& dev, float r1, float r2, int n1, int n2 );
Mesh * loadMesh      ( Device& dev, const char * fileName, float scale = 1.0f );
Mesh * loadMesh      ( Device& dev, const char * fileName, const glm::mat3& scale, const glm::vec3& offs );

#endif
