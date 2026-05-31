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
	mat4 model;
	mat4 view;
	mat4 proj;
	mat3 nm;
};

struct Vertex
{
	vec3	pos;
	vec3	normal;
	vec2	texCoord;
};

	// per-primitive data in indices
struct Primitive
{
	uint32_t	start;		// starting index
	uint32_t	count;
};

template <>
inline GraphicsPipeline&	registerVertexAttrs<Vertex> ( GraphicsPipeline& pipeline ) 
{
	return pipeline
		.addVertexAttr ( 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) )		// binding, location, format, offset
		.addVertexAttr ( 0, 1, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, texCoord) )
		.addVertexAttr ( 0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) );
}

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	Sampler							sampler;
	Buffer							vertexBuffer, indexBuffer;
	std::vector<Vertex>				vertices;
	std::vector<uint32_t>			indices;
	std::vector<Primitive>			primitives;
	std::vector<Texture *>			textures;

public:
	ExampleWindow ( int w, int h, const std::string& t, const std::string& path, const std::string& fileName ) : VulkanWindow ( w, h, t )
	{
		setController ( new RotateController ( this, glm::vec3(2.0f, 2.0f, 2.0f) ) );

		loadModel ( path, fileName );

		sampler.create  ( device );		// use default options	
		createPipelines ();
	}

	~ExampleWindow()
	{
		for ( size_t i = 0; i < textures.size (); i++ )
			delete textures [i];
	}

	bool	loadModel ( const std::string& path, const std::string& fileName )
	{
		tinygltf::TinyGLTF	loader;
		tinygltf::Model		model;
		std::string			err;
		std::string			warn;

		bool importResult = loader.LoadASCIIFromFile ( &model, &err, &warn, (path + fileName).c_str () );

		if ( !err.empty () || !importResult )
		{
			fatal () << "Error loading gltf model." << std::endl;

			return false;
		}

		if ( !warn.empty() )
			log () << warn << std::endl;

		bindModel ( path, model );

		return true;
	}

	void	bindModel ( const std::string& path, tinygltf::Model& model )
	{
		assert ( model.buffers.size () == 1 );

		const tinygltf::Scene &scene = model.scenes [model.defaultScene];

		for ( size_t i = 0; i < scene.nodes.size(); ++i )
		{
			assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size ()));

			bindModelNodes ( model, model.nodes [scene.nodes [i]] );
		}

		vertexBuffer.create  ( device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, Buffer::hostWrite );
		indexBuffer.create   ( device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  indices,  Buffer::hostWrite );

			// load textures
		for ( size_t i = 0 ; i < model.textures.size (); i++ )
		{
			tinygltf::Texture& tex = model.textures [i];
			tinygltf::Image&   img = model.images   [tex.source];

			Texture * texture = new Texture;

			if ( img.image.empty () )
				texture -> load ( device, path + img.uri );
			else
					// use already loaded image data
				texture -> loadRaw ( device, img.width, img.height, img.image.data (), img.component == 4 ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8_UNORM, true );

			textures.push_back ( texture );
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
		for ( size_t i = 0; i < mesh.primitives.size (); ++i )
		{
			tinygltf::Primitive primitive     = mesh.primitives [i];
			tinygltf::Accessor  indexAccessor = model.accessors [primitive.indices];
			size_t				indexOffset   = model.bufferViews [indexAccessor.bufferView].byteOffset + indexAccessor.byteOffset;

			primitives.push_back ( Primitive { uint32_t ( indices.size () ), uint32_t ( indexAccessor.count ) } );

				// store and convert indices to uint32_t format
			if ( indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE )		// byte index
			{ 
				uint8_t * ptr = (uint8_t *)(&model.buffers [0].data.at ( 0 ) + indexOffset);

				for ( size_t i = 0; i < indexAccessor.count; i++ )
					indices.push_back ( *ptr++ );
			}
			else
			if ( indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT )	// uint16_t index
			{ 
				uint16_t * ptr = (uint16_t *)(&model.buffers [0].data.at ( 0 ) + indexOffset);

				for ( size_t i = 0; i < indexAccessor.count; i++ )
					indices.push_back ( *ptr++ );
			}
			else
			if ( indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT )	// uint32_t index
			{ 
				uint32_t * ptr = (uint32_t *)(&model.buffers [0].data.at ( 0 ) + indexOffset);

				for ( size_t i = 0; i < indexAccessor.count; i++ )
					indices.push_back ( *ptr++ );
			}

				// append vertex data
			const float * positionBuffer  = nullptr;
			const float * normalsBuffer   = nullptr;
			const float * texCoordsBuffer = nullptr;

			for ( auto &attrib : primitive.attributes )
			{
				tinygltf::Accessor accessor   = model.accessors [attrib.second];
				int				   byteStride = accessor.ByteStride ( model.bufferViews [accessor.bufferView] );
				VkDeviceSize	   offset     = model.bufferViews [accessor.bufferView].byteOffset + accessor.byteOffset;

				if ( attrib.first.compare ( "POSITION" ) == 0 )
					positionBuffer = (const float *)(&model.buffers [0].data.at(0) + offset);
				else
				if ( attrib.first.compare ( "NORMAL" ) == 0 )
					normalsBuffer = (const float *)(&model.buffers [0].data.at(0) + offset);
				else
				if ( attrib.first.compare ( "TEXCOORD_0" ) == 0 )
					texCoordsBuffer = (const float *)(&model.buffers [0].data.at(0) + offset);
			}

			assert ( positionBuffer != nullptr && normalsBuffer != nullptr && texCoordsBuffer != nullptr );

			tinygltf::Accessor	posAccessor = model.accessors [primitive.attributes ["POSITION"]];

			for ( uint32_t i = 0; i < posAccessor.count; i++ )
			{
				Vertex	v;

				v.pos      = glm::vec3 ( positionBuffer  [3*i + 0], positionBuffer  [3*i + 1], positionBuffer [3*i + 2] );
				v.normal   = glm::vec3 ( normalsBuffer   [3*i + 0], normalsBuffer   [3*i + 1], normalsBuffer  [3*i + 2] );
				v.texCoord = glm::vec2 ( texCoordsBuffer [2*i + 0], texCoordsBuffer [2*i + 1] );

				vertices.push_back ( v );
			}
		}
	}

	glm::mat4 transformMatrixFromNode ( const tinygltf::Node &node )
	{
		glm::mat4	m ( 1.0f );		// set to unit matrix

		if ( !node.matrix.empty () )
		{
			std::transform ( node.matrix.begin (), node.matrix.end (), glm::value_ptr ( m ), [](double v) { return float ( v ); } );
		}
		
		if ( !node.scale.empty () )
		{
			glm::vec3	scale = glm::vec3 ( float ( node.scale [0] ), float ( node.scale [1] ), float ( node.scale [2] ) );

			glm::scale ( m, scale );				
		}

		if( !node.rotation.empty () )
		{
			glm::quat	rotation = glm::quat ( float ( node.rotation [0] ), float ( node.rotation [1] ), float ( node.rotation [2] ), float ( node.rotation [3] ) );

			m = m * glm::mat4_cast ( rotation );
		}

		if( !node.translation.empty () )
		{
			glm::vec3	translation = glm::vec3 ( float ( node.translation [0] ), float ( node.translation [1] ), float ( node.translation [2] ) );
				
			glm::translate ( m, translation );
		}

		return m;
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
				.addImage         ( 1, *(textures [0]), sampler)
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
				.addVertexBinding  ( sizeof ( Vertex ) )
				.addVertexAttributes <Vertex> ()
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
				.bindVertexBuffers ( { {vertexBuffer, 0} } )
				.bindIndexBuffer   ( indexBuffer, VK_INDEX_TYPE_UINT32 )
				.addDescriptorSets ( { descriptorSets [i] } )
				.setViewport       ( swapChain.getExtent () )
				.setScissor        ( swapChain.getExtent () );

			for ( auto& p : primitives )
				commandBuffers [i].drawIndexed ( p.count, 1, p.start );

			commandBuffers [i].end();
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
	return ExampleWindow ( 800, 600, "Simple mesh", "models/gltf/", "Cube.gltf" ).run ();
}
