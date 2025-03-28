#include	<memory>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION

#include	<tiny_gltf.h>
#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Mesh.h"
#include	"Controller.h"

struct Ubo 
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat3 nm;
};

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	Texture							texture;
	Sampler							sampler;
	std::unique_ptr<Mesh>           mesh;

	Buffer							vertexBuffer, indexBuffer;

	struct Binding	// single attribute binding to vertexBuffer
	{ 
		int				va;		// attribute no (location)
		VkFormat		format;
		VkDeviceSize	stride;
		VkDeviceSize	offset;
	};

	std::vector<Binding>	attribBindings;
	std::vector<uint8_t>	indexData;

public:
	ExampleWindow ( int w, int h, const std::string& t ) : VulkanWindow ( w, h, t )
	{
		setController ( new RotateController ( this, glm::vec3(2.0f, 2.0f, 2.0f) ) );

		loadModel ( "models/gltf/Cube.gltf ");

		sampler.create  ( device );		// use default optiona		
		createPipelines ();
	}

	bool	loadModel( const std::string& fileName )
	{
		tinygltf::TinyGLTF	loader;
		tinygltf::Model		model;
		std::string			err;
		std::string			warn;

		bool importResult = loader.LoadASCIIFromFile ( &model, &err, &warn, fileName.c_str());

		if ( !err.empty() )
		{
			fatal () << "Error loading gltf model." << std::endl;

			return false;
		}

		if ( !warn.empty() )
			log () << warn << std::endl;

		if ( !importResult )
		{
			fatal () << "Failed to load gltf file." << std::endl;

			return false;
		}

		bindModel ( model );

		return true;
	}

	void	appendIndexData ( tinygltf::Model& model,  size_t offs, size_t size )
	{
		uint8_t * ptr = model.buffers [0].data.at ( 0 ) + offs;

		indexData.reserve ( bufferData.size () + numBytes );

		for ( size_t i = 0; i < size; i++ )
			indexData.push_back ( *ptr++ );
	}

	inline	int typeSize ( int type )
	{
		switch ( type )
		{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return 1;

			case TINYGLTF_COMPONENT_TYPE_SHORT:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return 2;

			case TINYGLTF_COMPONENT_TYPE_INT:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return 4;

			default:
				break;

		}

		return 0;
	}
	inline VkFormat attributeFormat ( tinygltf::Accessor& accessor )
	{
		switch ( accessor.componentType )
		{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
			{
				static const std::map<int, VkFormat> mapped_format = 
				{
					{ TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_SINT },
					{ TINYGLTF_TYPE_VEC2,   VK_FORMAT_R8G8_SINT },
					{ TINYGLTF_TYPE_VEC3,   VK_FORMAT_R8G8B8_SINT },
					{ TINYGLTF_TYPE_VEC4,   VK_FORMAT_R8G8B8A8_SINT }
				};

				return mapped_format.at ( accessor.type );

				break;
			}

			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			{
				static const std::map<int, VkFormat> mapped_format = 
				{
					{ TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_UINT },
					{ TINYGLTF_TYPE_VEC2,   VK_FORMAT_R8G8_UINT },
					{ TINYGLTF_TYPE_VEC3,   VK_FORMAT_R8G8B8_UINT },
					{ TINYGLTF_TYPE_VEC4,   VK_FORMAT_R8G8B8A8_UINT }
				};

				static const std::map<int, VkFormat> mapped_format_normalize = 
				{
					{ TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_UNORM },
					{ TINYGLTF_TYPE_VEC2,   VK_FORMAT_R8G8_UNORM },
					{ TINYGLTF_TYPE_VEC3,   VK_FORMAT_R8G8B8_UNORM },
					{ TINYGLTF_TYPE_VEC4,   VK_FORMAT_R8G8B8A8_UNORM }
				};

				if (accessor.normalized)
					return mapped_format_normalize.at ( accessor.type );
				else
					return mapped_format.at(accessor.type);

				break;
			}

			case TINYGLTF_COMPONENT_TYPE_SHORT:
			{
				static const std::map<int, VkFormat> mapped_format = 
				{
					{ TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_SINT },
					{ TINYGLTF_TYPE_VEC2,   VK_FORMAT_R8G8_SINT },
					{ TINYGLTF_TYPE_VEC3,   VK_FORMAT_R8G8B8_SINT },
					{ TINYGLTF_TYPE_VEC4,   VK_FORMAT_R8G8B8A8_SINT }
				};

				return mapped_format.at(accessor.type);

				break;
			}

			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			{
				static const std::map<int, VkFormat> mapped_format = 
				{
					{ TINYGLTF_TYPE_SCALAR, VK_FORMAT_R16_UINT },
					{ TINYGLTF_TYPE_VEC2,   VK_FORMAT_R16G16_UINT },
					{ TINYGLTF_TYPE_VEC3,   VK_FORMAT_R16G16B16_UINT },
					{ TINYGLTF_TYPE_VEC4,   VK_FORMAT_R16G16B16A16_UINT }
				};

				static const std::map<int, VkFormat> mapped_format_normalize = 
				{
					{ TINYGLTF_TYPE_SCALAR, VK_FORMAT_R16_UNORM },
					{ TINYGLTF_TYPE_VEC2,   VK_FORMAT_R16G16_UNORM },
					{ TINYGLTF_TYPE_VEC3,   VK_FORMAT_R16G16B16_UNORM },
					{ TINYGLTF_TYPE_VEC4,   VK_FORMAT_R16G16B16A16_UNORM }
				};

				if ( accessor.normalized )
					return mapped_format_normalize.at ( accessor.type );
				else
					return mapped_format.at(accessor.type);

				break;
			}

			case TINYGLTF_COMPONENT_TYPE_INT:
			{
				static const std::map<int, VkFormat> mapped_format = 
				{
					{ TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_SINT },
					{ TINYGLTF_TYPE_VEC2,   VK_FORMAT_R32G32_SINT },
					{ TINYGLTF_TYPE_VEC3,   VK_FORMAT_R32G32B32_SINT },
					{ TINYGLTF_TYPE_VEC4,   VK_FORMAT_R32G32B32A32_SINT }
				};

				return mapped_format.at ( accessor.type );

				break;
			}

			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			{
				static const std::map<int, VkFormat> mapped_format = 
				{
					{ TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_UINT },
					{ TINYGLTF_TYPE_VEC2,   VK_FORMAT_R32G32_UINT },
					{ TINYGLTF_TYPE_VEC3,   VK_FORMAT_R32G32B32_UINT },
					{ TINYGLTF_TYPE_VEC4,   VK_FORMAT_R32G32B32A32_UINT }
				};

				return mapped_format.at ( accessor.type );

				break;
			}

			case TINYGLTF_COMPONENT_TYPE_FLOAT:
			{
				static const std::map<int, VkFormat> mapped_format = 
				{
					{ TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_SFLOAT },
					{ TINYGLTF_TYPE_VEC2,   VK_FORMAT_R32G32_SFLOAT },
					{ TINYGLTF_TYPE_VEC3,   VK_FORMAT_R32G32B32_SFLOAT },
					{ TINYGLTF_TYPE_VEC4,   VK_FORMAT_R32G32B32A32_SFLOAT }
				};

				return mapped_format.at ( accessor.type );

				break;
			}

			default:
				return VK_FORMAT_UNDEFINED;
				break;
		}
	}

	void	bindModel ( tinygltf::Model& model )
	{
		assert ( model.buffers.size () == 1 );

		vertexBuffer.create  ( device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, model.buffers [0].data, Buffer::hostWrite);
		//indexBuffer.create   ( device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  indices,  Buffer::hostWrite );

		const tinygltf::Scene &scene = model.scenes [model.defaultScene];

		for ( size_t i = 0; i < scene.nodes.size(); ++i )
		{
			assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size ()));

			bindModelNodes ( model, model.nodes [scene.nodes [i]] );
		}
	}

	void bindModelNodes ( tinygltf::Model &model, tinygltf::Node &node )
	{
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
			bindMesh ( model, model.meshes [node.mesh] );

		for ( size_t i = 0; i < node.children.size(); i++ )
		{
			assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));

			bindModelNodes ( model, model.nodes [node.children [i]] );
		}
	}

	void bindMesh ( tinygltf::Model &model, tinygltf::Mesh &mesh )
	{
		for ( size_t i = 0; i < mesh.primitives.size(); ++i )
		{
			tinygltf::Primitive primitive     = mesh.primitives [i];
			tinygltf::Accessor  indexAccessor = model.accessors [primitive.indices];
			VkFormat			indexFormat   = attributeFormat ( indexAccessor );
			int					indexSize     = typeSize        ( indexAccessor.type) * indexAccessor.count;
			int					indexOffset   = model.bufferViews[indexAccessor.bufferView].byteOffset + indexAccessor.byteOffset;

			appendIndexData ( model, indexOffset, indexSize );

				// append index data (bufferViews[indexAccessor.bufferView].byteOffset + indexAccessor.byteOffset, count = indexAccessor.count * typeSize ( indexAccessor.type )
			for ( auto &attrib : primitive.attributes )
			{
				tinygltf::Accessor accessor   = model.accessors [attrib.second];
				int				   byteStride = accessor.ByteStride ( model.bufferViews [accessor.bufferView] );
				int				   size       = accessor.type != TINYGLTF_TYPE_SCALAR ? accessor.type : 1;
				int				   va         = -1;
				VkDeviceSize	   offset     = model.bufferViews [accessor.bufferView].byteOffset + accessor.byteOffset;

				if ( attrib.first.compare ( "POSITION" ) == 0 )
					va = 0;
				else
				if ( attrib.first.compare ( "NORMAL" ) == 0 )
					va = 1;
				else
				if ( attrib.first.compare ( "TEXCOORD_0" ) == 0 )
					va = 2;

				if ( va == -1 )
					continue;

				attribBindings.push_back ( { va, format, byteStride, offset } );
				// 	GraphicsPipeline::addVertexAttr ( uint32_t binding, uint32_t location, VkFormat format, uint32_t offset )
				// binding = 0, location = va, format = attributeFormat, offset = accessor.byteOffset, stride = byteStride
			}

			for ( size_t i = 0 ; i < model.textures.size (); i++ )
			{
				tinygltf::Texture& tex = model.textures [i];
				tinygltf::Image&   img = model.images [tex.source];

				// bool	Texture :: loadRaw ( Device& dev, int texWidth, int texHeight, const void * pixels, VkFormat format, bool mipmaps )
				// fmt       = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM; VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8_UNORM

/*
				if ( tex.source > -1 )
				{
					GLuint texid;
					glGenTextures(1, &texid);

					tinygltf::Image &image = model.images [tex.source];

					glBindTexture(GL_TEXTURE_2D, texid);
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

					GLenum format = GL_RGBA;

					if (image.component == 1) {
						format = GL_RED;
					} else if (image.component == 2) {
						format = GL_RG;
					} else if (image.component == 3) {
						format = GL_RGB;
					} else {
						// ???
					}

					GLenum type = GL_UNSIGNED_BYTE;
					if (image.bits == 8) {
						// ok
					} else if (image.bits == 16) {
						type = GL_UNSIGNED_SHORT;
					} else {
						// ???
					}

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
						format, type, &image.image.at(0));
				}
*/
			}
		}
	}

	void	createUniformBuffers ()
	{
		uniformBuffers.resize ( swapChain.imageCount() );
		
		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].create ( device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT );
	}

	void	freeUniformBuffers ()
	{
		uniformBuffers.clear ();
	}

	void	createDescriptorSets ()
	{
		descriptorSets.resize ( swapChain.imageCount () );

		for ( uint32_t i = 0; i < swapChain.imageCount (); i++ )
		{
			descriptorSets  [i]
				.setLayout        ( device, descAllocator, pipeline.getDescLayout () )
				.addUniformBuffer ( 0, uniformBuffers [i], 0, sizeof ( Ubo ) )
				.addImage         ( 1, texture, sampler )
				.create           ();
		}
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		pipeline.setDevice ( device )
				.setVertexShader   ( "shaders/shader-tex.vert.spv" )
				.setFragmentShader ( "shaders/shader-tex.frag.spv" )
				.setSize           ( swapChain.getExtent ().width, swapChain.getExtent ().height )
				.addVertexBinding  ( sizeof ( BasicVertex ) )
				.addVertexAttributes <BasicVertex> ()
				.addDescLayout     ( 0, DescSetLayout ()
					.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT )
					.add ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT ) )
				.setCullMode       ( VK_CULL_MODE_NONE               )
			.setDepthTest      ( true )
			.setDepthWrite     ( true )
			.create            ( renderPass );			

				// create before command buffers
		swapChain.createFramebuffers ( renderPass, depthTexture.getImageView () );

		createDescriptorSets ();
		createCommandBuffers ( renderPass );
	}

	virtual	void	freePipelines () override
	{
		commandBuffers.clear ();
		pipeline.clean       ();
		renderPass.clean     ();
		freeUniformBuffers   ();
		descriptorSets.clear ();
		descAllocator.clean  ();
	}
	
	virtual	void	submit ( uint32_t imageIndex ) override 
	{
		updateUniformBuffer ( imageIndex );
		defaultSubmit       ( commandBuffers [imageIndex] );
	}

	void	createCommandBuffers ( Renderpass& renderPass )
	{
		auto&	framebuffers = swapChain.getFramebuffers ();

		commandBuffers = device.allocCommandBuffers ( (uint32_t)framebuffers.size ());

		for ( size_t i = 0; i < commandBuffers.size(); i++ )
		{
			commandBuffers [i]
				.begin             ()
				.beginRenderPass   ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( swapChain.getExtent ().width, swapChain.getExtent ().height ).clearColor ().clearDepthStencil () )
				.pipeline          ( pipeline )
				.addDescriptorSets ( { descriptorSets[i] } )
				.setViewport       ( swapChain.getExtent () )
				.setScissor        ( swapChain.getExtent () )
				.render            ( mesh.get () )
				.end               ();
		}
	}

	void updateUniformBuffer ( uint32_t currentImage )
	{
		uniformBuffers [currentImage]->model = controller->getModelView  ();
		uniformBuffers [currentImage]->view  = glm::mat4 ( 1 );
		uniformBuffers [currentImage]->proj  = controller->getProjection ();
	}
};

int main ( int argc, const char * argv [] ) 
{
	return ExampleWindow ( 800, 600, "Simple mesh" ).run ();
}
