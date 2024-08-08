#pragma once

//#include	"BasicMesh.h"
//#include	"SkinnedMesh.h"
#include	"bbox.h"
#include	<glm/gtc/type_ptr.hpp>			// for make_mat
#include	<glm/glm.hpp>
#include	<glm/gtc/matrix_transform.hpp>

#include	<assimp/Importer.hpp> 
#include	<assimp/scene.h>     
#include	<assimp/postprocess.h>
#include	<assimp/DefaultLogger.hpp>

class	MeshLoader
{
	Assimp::Importer 	importer;
	//unsigned int		flags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;	// | aiProcess_PopulateArmatureData;
        //unsigned int            flags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices;

	const unsigned int     	flags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
		// aiProcess_ValidateDataStructure 
		// aiProcess_LimitBoneWeights

public:
	MeshLoader ()
	{
		importer.SetPropertyBool ( AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, true );

		setLog ( "assimp.log" );
	}
	
	MeshLoader&	setLog ( const std::string& logFile )
	{
		Assimp::DefaultLogger::create ( logFile.c_str (),  Assimp::Logger::VERBOSE, aiDefaultLogStream_FILE );
		
		return *this;
	}
/*
	SkinnedMesh *	load ( const std::string& fileName, const std::string& texturePath, float scale = 1 )
	{
		const aiScene  * scene = importer.ReadFile ( fileName, flags );
	
		if ( scene == nullptr )
			return nullptr;
		
		return new SkinnedMesh ( scene, texturePath, scale );
	}
*/
	const aiScene * loadScene ( const std::string& fileName, bool loadBones = false )
	{
		return importer.ReadFile ( fileName.c_str (), flags | ( loadBones ? aiProcess_PopulateArmatureData | aiProcess_LimitBoneWeights: 0 ) );
	}
	
	template <typename Vertex, typename Index = uint32_t>
	static void loadAiMesh ( const aiMesh * mesh, float scale, std::vector<Vertex>& vertices, std::vector<Index>& indices, bbox& box, int base = 0 )
	{
		const aiVector3D	zero3D ( 0.0f, 0.0f, 0.0f );
			
		for ( size_t i = 0; i < mesh->mNumVertices; i++ ) 
		{
			aiVector3D&  	pos    = mesh->mVertices[i];
			aiVector3D&  	normal = mesh->mNormals[i];
			aiVector3D   	tex;
			aiVector3D   	tangent, binormal;
			Vertex			v;
				
			memset ( &v, 0, sizeof ( v ) );

			if ( mesh->HasTextureCoords (0) )
				tex = mesh->mTextureCoords [0][i];
			else
				tex = zero3D;
				
			tangent  = (mesh->HasTangentsAndBitangents()) ? mesh->mTangents   [i] : zero3D;
			binormal = (mesh->HasTangentsAndBitangents()) ? mesh->mBitangents [i] : zero3D;
			v.pos    = scale * glm::vec3 ( pos.x, pos.y, pos.z );
			v.tex    = glm::vec2 ( tex.x, 1.0f - tex.y );
			v.n      = glm::vec3 ( normal.x, normal.y, normal.z );
			v.t      = glm::vec3 ( tangent.x, tangent.y, tangent.z );
			v.b      = glm::vec3 ( binormal.x, binormal.y, binormal.z );

			box.addVertex ( v.pos );		

			vertices.push_back ( v );
		}

		for ( size_t i = 0; i < mesh->mNumFaces; i++ ) 
		{
			const aiFace& face = mesh->mFaces [i];
				
			assert ( face.mNumIndices == 3 );
			
			indices.push_back( base + face.mIndices[0] );
			indices.push_back( base + face.mIndices[1] );
			indices.push_back( base + face.mIndices[2] );
		}
	}
/*			
	static void	loadMaterials ( const aiScene * scene, std::vector<BasicMaterial*>& materials )
	{
		for ( size_t i = 0; i < scene->mNumMaterials; i++ )
		{
			const aiMaterial*  material = scene->mMaterials[i];
			BasicMaterial	 * mat = new BasicMaterial;
			aiString		   name;
			std::string		   path = "";
			aiColor4D		   color;
			glm::vec4		   diffColor = glm::vec4 ( 0 );
				

			material-> Get ( AI_MATKEY_NAME, name );	
			mat     -> setName ( name.C_Str());
			
			if ( material->GetTextureCount(aiTextureType_DIFFUSE) > 0 )
			{
				if ( material->GetTexture(aiTextureType_DIFFUSE, 0, &name, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS )
				{
					printf ( "\tDiffuse %s\n", name.C_Str () );
					mat->setDiffuseMap ( path + name.C_Str() );
				}
			}
			else
			{
				if( AI_SUCCESS == material->Get ( AI_MATKEY_COLOR_DIFFUSE, color ) )	// color4_to_float4(&diffuse, colour);  //if it exists copy the elements into the array
				{
					diffColor = glm::vec4 ( color.r, color.g, color.b, color.a );
					
					printf ( "got diff color (%f %f %f)\n", diffColor.x, diffColor.y, diffColor.z );
					
					mat->setDiffColor ( diffColor );
				}
			}
			
			if ( material->GetTextureCount(aiTextureType_SPECULAR) > 0 )
				if ( material->GetTexture(aiTextureType_SPECULAR, 0, &name, nullptr, nullptr, nullptr, nullptr, nullptr ) == AI_SUCCESS)
				{
					printf ( "\tSpecular %s\n", name.C_Str () );
					mat->setSpecMap ( path + name.C_Str () );
				}

			if ( material->GetTextureCount(aiTextureType_NORMALS) > 0 )
				if ( material->GetTexture(aiTextureType_NORMALS, 0, &name, NULL, NULL, NULL, NULL, NULL ) == AI_SUCCESS )
				{
					printf ( "\tNormals %s\n", name.C_Str () );
					mat->setBumpMap ( path + name.C_Str () );
				}

			if ( material->GetTextureCount(aiTextureType_SHININESS) > 0 )
				if ( material->GetTexture(aiTextureType_SHININESS, 0, &name, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS )
				{
					printf ( "\tShininess %s\n", name.C_Str () );
				}

			if ( material->GetTextureCount(aiTextureType_HEIGHT) > 0 )
				if ( material->GetTexture(aiTextureType_HEIGHT, 0, &name, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS )
				{
					printf ( "\tHeight %s\n", name.C_Str () );
					mat->setBumpMap ( path + name.C_Str () );
				}

			materials.push_back ( mat );
		}
	}
*/	
	
};

// helper functions for Assimp converting
inline std::string	toString ( const aiString& s )
{
	return std::string ( s.C_Str () );
}

inline glm::vec3 toGlmVec3 ( const aiVector3D &v )
{ 
	return glm::vec3(v.x, v.y, v.z); 
}

inline glm::vec2 toGlmVec2 ( const aiVector2D &v )
{ 
	return glm::vec2(v.x, v.y); 
}

inline glm::quat toGlmQuat ( const aiQuaternion& q )
{ 
	return glm::quat ( q.w, q.x, q.y, q.z ); 
}

inline glm::mat4 toGlmMat4 ( const aiMatrix4x4 &m )
{ 
	return glm::transpose(glm::make_mat4(&m.a1)); 
}

inline glm::mat3 toGlmMat3 ( const aiMatrix3x3 &m ) 
{ 
	return glm::transpose(glm::make_mat3(&m.a1)); 
}

inline std::string	getAiTexture ( const aiMaterial * material, int textureType )
{
	aiString	name;

	if ( material->GetTextureCount ( (aiTextureType) textureType ) > 0 )
		if ( material->GetTexture ( (aiTextureType) textureType, 0, &name, nullptr, nullptr, nullptr, nullptr, nullptr ) == AI_SUCCESS )
			return name.C_Str ();

	return "";
}
