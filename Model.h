#include	"Texture.h"
#include	"AssimpMeshLoader.h"

inline float max3 ( const glm::vec3& v )
{
	float	m1 = v.x > v.y ? v.x : v.y;

	return v.z > m1 ? v.z : m1;
}

struct PushConstants
{
	glm::mat4	matrix;									// node transform matrix
	uint32_t	albedo, metallic, normal, roughness;	// indices into textures array
};

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

struct	PbrMaterial
{
	std::string		name;
	uint32_t		albedo, metallic, normal, roughness;
	
public:
	PbrMaterial ( Device& device, std::vector<Texture>& textures, const std::string& nm, const std::string& path, const std::string& prfx ) : name ( nm )
	{
		std::string	prefix = prfx;
		auto		pos    = prefix.find ( '*' );

		if ( pos != std::string::npos )	// replace * with name
			prefix = prefix.substr ( 0, pos ) + name;

		uint32_t	index = (uint32_t)textures.size ();

		albedo    = index;
		metallic  = index + 1;
		normal    = index + 2;
		roughness = index + 3;

			// create albedo texture (index)
		textures.emplace_back ( Texture () );

		if ( !textures [albedo].load ( device, path + "/" + prefix + "_BaseColor.png" ) )
			printf ( "Error loading albedo texture %s\n", path.c_str () );
		
			// create metallic texture (index+1)
		textures.emplace_back ( Texture () );

		textures [metallic].load ( device, path + "/" + prefix + "_Metallic.png"  );

			// create normal texture (index+2)
		textures.emplace_back ( Texture () );

		textures [normal].load ( device, path + "/" + prefix + "_Normal.png"    );

			// create roughness texture (index+3)
		textures.emplace_back ( Texture () );

		textures [roughness].load ( device, path + "/" + prefix + "_Roughness.png" );
		
		printf ( "Material: %s\n", name.c_str () );
		printf ( "\tLoading %s \n", (path + "/" + prefix + "_BaseColor.png").c_str () );
		printf ( "\tLoading %s \n", (path + "/" + prefix + "_Metallic.png").c_str () );
		printf ( "\tLoading %s \n", (path + "/" + prefix + "_Normal.png").c_str () );
		printf ( "\tLoading %s \n", (path + "/" + prefix + "_Roughness.png").c_str () );
	}

	const std::string&	getName () const
	{
		return name;
	}
};

struct Primitive
{
	PbrMaterial   * material    = nullptr;
	int				materialNo  = -1;
	int				firstIndex  = 0;
	int				indexCount  = 0;
	int				firstVertex = 0;
	bbox			bounds;
	
	void	render ( CommandBuffer& cb ) const
	{
		cb.drawIndexed ( indexCount, 1, firstIndex, 0, 0 );
		//glDrawElements ( GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (const void *) (firstIndex*sizeof(GLuint)) );
	}
};

struct	Node
{
	std::string				name;
	glm::mat4				transform = glm::mat4 ( 1.0f );
	Node                  * parent    = nullptr;
	std::vector<Node *>		children;
	std::vector<Primitive*> meshes;
	bbox					bounds;
	
	Node ( const std::string& n, const glm::mat4& m ) : name ( n ), transform ( m ) {}
	
	const bbox&	getBox () const
	{
		return bounds;
	}
	
	void	computeBounds ()
	{
		bounds.reset ();
		
			// get bounds for all meshes
		for ( auto * mesh : meshes )
			bounds.merge ( mesh->bounds );
		
			// collect bounds from children
		for ( auto * child : children )
		{
			child->computeBounds ();
			
			bounds.merge ( child->bounds );
		}
		
			// apply transform to them
		bounds.apply ( transform );
	}
	
	glm::mat4	parentTransform ( const glm::mat4& m ) const
	{
		if ( parent != nullptr )
		{
			//m = transform * m;
			
			return parent->parentTransform ( transform * m );
		}
		
		return m;
	}
};

class	Model
{
	std::string					name;
	Buffer						vertexBuf;
	Buffer						indexBuf;
	std::vector<Primitive *>	meshes;
	std::vector<PbrMaterial *>	materials;
	std::vector<Texture>		textures;
	Node					  * root = nullptr;
	
public:
	Model () = default;
	
	bool	load ( Device& device, const std::string& fileName, const std::string& texturePath, const std::string& prefix )
	{	
		MeshLoader	loader;
		auto      * scene = loader.loadScene ( fileName );

		loadMaterials ( device, loader, scene, texturePath, prefix );
		loadMeshes    ( device, loader, scene, 1  );
		loadNodes     ( loader, scene     );
		
		return true;
	}
	
	const std::vector<Texture>&	getTextures () const
	{
		return textures;
	}

	void render ( GraphicsPipeline& pipeline, CommandBuffer& cb, const glm::mat4& matrix )
	{
		cb.bindVertexBuffers ( {{ vertexBuf, 0 }} );
		cb.bindIndexBuffer ( indexBuf, VK_INDEX_TYPE_UINT32 );

		renderNode ( root, pipeline, cb, matrix );
	}
	
	void	renderNode ( Node * node, GraphicsPipeline& pipeline, CommandBuffer& cb, const glm::mat4& matrix )
	{
		glm::mat4	tr   = glm::inverse ( root->transform ) * node->parentTransform ( glm::mat4 ( 1 ) );
		glm::mat4	mv   = matrix * tr;
		//glm::mat3	nm   = normalMatrix ( mv );
		glm::vec3	sz   = root->getBox ().getSize   ();
		float     	size = max3 ( sz ) / 7;
		glm::mat4	translate = glm::translate ( glm::mat4(1), -root->getBox ().getCenter () );
		glm::mat4	scale     = glm::scale     ( glm::mat4(1), glm::vec3 ( 1.0f / size ) );

		//program.setUniformMatrix ( "mv", scale * translate * mv );
		//program.setUniformMatrix ( "nm", nm );

		PushConstants	push { mv, 0, 0, 0, 0 };

		for ( auto * mesh : node->meshes )
		{
			push.matrix    = mv;
			push.albedo    = mesh->material->albedo;
			push.metallic  = mesh->material->metallic;
			push.normal    = mesh->material->normal;
			push.roughness = mesh->material->roughness;

			cb.pushConstants ( pipeline.getLayout (), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, push );

			mesh -> render ( cb );
		}
		
		for ( auto * c : node->children )
			renderNode ( c, pipeline, cb, node->transform * matrix );
	}
	
	Node * createNode ( const aiNode * n, Node * parent )
	{
		auto node = new Node ( toString ( n->mName ), toGlmMat4 ( n->mTransformation ) );
		
		node->parent = parent;
		
				// add meshes (mNumMeshes, mMeshes (int indices into meshes list)
		for (unsigned  int i = 0; i < n->mNumMeshes; i++ )
			node->meshes.push_back ( meshes [n->mMeshes[i]] );
		
				// add children
		for ( unsigned int i = 0; i < n->mNumChildren; i++ )
			node->children.push_back ( createNode ( n->mChildren [i], node ) );
		
		return node;
	}
	
	void	loadNodes ( MeshLoader& loader, const aiScene * scene )
	{
		root = createNode ( scene->mRootNode, nullptr );

		root->computeBounds ();	
	}
	
	void	loadMeshes ( Device& device, MeshLoader& loader, const aiScene * scene, float scale )
	{
		bbox 						box;
		std::vector<BasicVertex>	vertices;
		std::vector<GLuint>			indices;
		int							base = 0;
			
				// Count the number of vertices and indices
		for ( uint32_t i = 0 ; i < scene->mNumMeshes ;i++ ) 
		{
			if ( scene->mMeshes [i]->mNormals == nullptr )		// sometimes happen
				continue;

			std::vector<BasicVertex>	vs;
			std::vector<GLint>			is;
			Primitive				  * mesh = new Primitive;
			
			loader.loadAiMesh<BasicVertex> ( scene->mMeshes[i], scale, vs, is, box, base );
			
			mesh->materialNo  = scene -> mMeshes [i] -> mMaterialIndex;
			mesh->firstIndex  = (int)indices.size  ();
			mesh->indexCount  = (int)is.size       ();
			mesh->firstVertex = (int)vertices.size ();
			mesh->material    = materials [mesh->materialNo];
			mesh->bounds      = box;

			meshes.push_back ( mesh );
			
			for ( auto& v : vs )
				vertices.push_back ( v );
			
			for ( auto i : is )
				indices.push_back ( i );			

			base = (int)vertices.size ();
		}	

		createBuffer ( device, vertexBuf, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
		createBuffer ( device, indexBuf,  indices,  VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
	}

	void	loadMaterials ( Device& device, MeshLoader& loader, const aiScene * scene, const std::string& path, const std::string& prefix )
	{
		textures.reserve ( 4 * scene->mNumMaterials );

		for ( unsigned i = 0; i < scene->mNumMaterials; i++ )
		{
			std::string	name = toString ( scene->mMaterials [i]->GetName() );
			aiString	texName;

			printf ( "---- Material: %s\n", name.c_str () );

			for ( int m = 0; m < aiTextureType_UNKNOWN; m++ )
				if ( scene->mMaterials[i]->GetTexture ( (aiTextureType)m, 0, &texName, nullptr, nullptr, nullptr, nullptr, nullptr ) == AI_SUCCESS )
					if ( *texName.C_Str () )
						printf ( "\tTEXTURE %d: %s\n", m, texName.C_Str () );
			
			PbrMaterial * mat  = new PbrMaterial ( device, textures, name, path, prefix );
			
			materials.push_back ( mat );
		}
	}

	// create buffer and fill using staging buffer
	template<typename T>
	void	createBuffer ( Device& device, Buffer& buffer, const std::vector<T>& data, uint32_t usage )
	{
		Buffer				stagingBuffer;
		SingleTimeCommand	cmd ( device );

			// use staging buffer to copy data to GPU-local memory
		stagingBuffer.create ( device, data.size () * sizeof ( T ), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, Buffer::hostWrite );
		stagingBuffer.copy   ( data.data (), data.size () * sizeof ( T ) );
		buffer.create        ( device, data.size () * sizeof ( T ), VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, 0 );
		buffer.copyBuffer    ( cmd, stagingBuffer, data.size () * sizeof ( T ) );
	}

};
