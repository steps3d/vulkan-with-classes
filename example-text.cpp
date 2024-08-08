#include	<memory>
#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Mesh.h"
#include	"stb_font_consolas_24_latin1.inl"
#include	"ScreenQuad.h"
#include	"Controller.h"

struct UniformBufferObject 
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat3 nm;
};

struct TextVertex 
{
	glm::vec4 pos;
};

template <>
GraphicsPipeline&	registerVertexAttrs<TextVertex> ( GraphicsPipeline& pipeline ) 
{
	return pipeline
		.addVertexAttr ( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TextVertex, pos)   )	;	// binding, location, format, offset
}

class	TextLayer
{
	stb_fontchar				stbFontData [STB_FONT_consolas_24_latin1_NUM_CHARS];
	const uint32_t				fontWidth    = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
	const uint32_t				fontHeight   = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
	glm::vec4				  * mapped       = nullptr;		// pointer to free space in buffer
	Device					  * device       = nullptr;
	VulkanWindow			  * window       = nullptr;
	int							numLetters   = 0;	
	int							maxLetters   = 0;			// max letters
	int							width        = 0;
	int							height       = 0;
	bool						dirty        = true;
	PersistentBuffer			textBuffer;			// persistent buffer to hold text to be rendered
	std::vector<CommandBuffer>	cmds;
	GraphicsPipeline			pipeline;
	Renderpass					renderPass;	// = nullptr;
	Texture						image;
	Sampler						sampler;
	DescriptorSet				descriptorSet;
	Semaphore					semaphore;

public:
	TextLayer ( VulkanWindow * win ) : window( win ) {}

	SwapChain&	getSwapChain ()
	{
		return window->getSwapChain ();
	}

	DescriptorAllocator&	getDescAllocator ()
	{
		return window->getDescriptorAllocator ();
	}

	Semaphore&	getSemaphore ()
	{
		return semaphore;
	}

	GraphicsPipeline&	getPipeline ()
	{
		return pipeline;
	}

	DescriptorSet&	getDescriptorSet ()
	{
		return descriptorSet;
	}

	bool	create ( Device * dev, int _maxLetters )
	{
		device     = dev;
		maxLetters = _maxLetters;
		numLetters = 0;
		width      = getSwapChain ().getWidth  ();
		height     = getSwapChain ().getHeight ();

		if ( !textBuffer.create ( *device, maxLetters * sizeof ( glm::vec4 ), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, Buffer::hostWrite ) )
			return false;

		mapped     = (glm::vec4 *)textBuffer.getPtr ();

		loadFont ();

		sampler.create ( *device );

		renderPass
			.addAttachment   ( getSwapChain ().getFormat (),                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                  VK_ATTACHMENT_LOAD_OP_DONT_CARE )
			.addAttachment   ( window->getDepthTexture ().getImage ().getFormat (), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_DONT_CARE )
			.addSubpass      ( 0 )
			.addDepthSubpass ( 1 )
			.create          ( *device );


		pipeline.setDevice ( *device )
			.setVertexShader   ( "shaders/text.vert.spv" )
			.setFragmentShader ( "shaders/text.frag.spv" )
			.setSize           ( width, height )
			.addVertexBinding  ( sizeof ( glm::vec4 ) )
			.addVertexAttr ( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 )		// binding, location, format, offset
			.addDescLayout     ( 0, DescSetLayout ()
				.add ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT ) )
			.setCullMode       ( VK_CULL_MODE_NONE               )
			.setDepthTest      ( false )
			.setDepthWrite     ( false )
			.setTopology       ( VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP )
			.create            ( renderPass );			

		descriptorSet
			.setLayout ( *device, getDescAllocator (), pipeline.getDescLayout () )
			.addImage  ( 1, image, sampler )
			.create    ();

		semaphore.create ( *device );

		return true;
	}

	void	loadFont ()
	{
		const uint32_t fontWidth  = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
		const uint32_t fontHeight = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;

		static unsigned char font24pixels[fontWidth][fontHeight];
		stb_font_consolas_24_latin1(stbFontData, font24pixels, fontHeight);

		image.loadRaw ( *device, fontWidth, fontHeight, &font24pixels[0][0], VK_FORMAT_R8_UNORM );

		log () <<  "Size " << fontWidth << " x " << fontHeight << std::endl;
	}

	void addText ( std::string str, float x, float y )
	{
		const uint32_t	firstChar = STB_FONT_consolas_24_latin1_FIRST_CHAR;
		const float		charW     = 1.5f / width;
		const float		charH     = 1.5f / height;
		const float		fbW       = (float) width;
		const float		fbH       = (float) height;

		assert(mapped != nullptr);

		dirty = true;
		x     = (x / fbW * 2.0f) - 1.0f;
		y     = (y / fbH * 2.0f) - 1.0f;

		// generate an uv mapped quad per char in the new text
		for ( auto ch : str )
		{
			// check for free space
			if ( numLetters >= maxLetters )
				break;

			stb_fontchar * charData = &stbFontData [(uint32_t)ch - firstChar];

			mapped->x = (x + (float)charData->x0 * charW);
			mapped->y = (y + (float)charData->y0 * charH);
			mapped->z = charData->s0;
			mapped->w = charData->t0;
			mapped++;

			mapped->x = (x + (float)charData->x1 * charW);
			mapped->y = (y + (float)charData->y0 * charH);
			mapped->z = charData->s1;
			mapped->w = charData->t0;
			mapped++;

			mapped->x = (x + (float)charData->x0 * charW);
			mapped->y = (y + (float)charData->y1 * charH);
			mapped->z = charData->s0;
			mapped->w = charData->t1;
			mapped++;

			mapped->x = (x + (float)charData->x1 * charW);
			mapped->y = (y + (float)charData->y1 * charH);
			mapped->z = charData->s1;
			mapped->w = charData->t1;
			mapped++;

			x += charData->advance * charW;

			numLetters++;
		}
	}

	void	clearText ()
	{
		dirty      = true;
		numLetters = 0;
		mapped     = (glm::vec4 *)textBuffer.getPtr ();
	}

	CommandBuffer&	getCommandBuffer ( int index )
	{
		if ( dirty )
		{
			dirty = false;

			for ( auto& cb : cmds )
				cb.clean ();

			updateCommandBuffers ();
		}

		return cmds [index];
	}

	void	updateCommandBuffers()
	{
		auto	framebuffers = getSwapChain ().getFramebuffers ();

		if ( cmds.size () != framebuffers.size () )
			cmds.resize ( framebuffers.size () );

		for ( int i = 0; i < framebuffers.size (); i++ )
		{
			cmds [i].create ( *device );

			cmds [i].begin ()
				.beginRenderPass   ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( width, height ) )
				.pipeline          ( pipeline )
				.bindVertexBuffers (  { {textBuffer, 0} } )
				.addDescriptorSets ( { descriptorSet } );

			for ( int j = 0; j < numLetters; j++ )
				cmds [i].draw ( 4, 1, 4 * j );

			cmds [i].end  ();
		}
	}
};

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	std::vector<Buffer>				uniformBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	Texture							texture;
	Sampler							sampler;
	std::unique_ptr<Mesh>           mesh;
	TextLayer						textLayer;

public:
	ExampleWindow ( int w, int h, const std::string& t ) : textLayer ( this ), VulkanWindow ( w, h, t )
	{
		setController ( new RotateController ( this, glm::vec3(2.0f, 2.0f, 2.0f) ) );

		mesh = std::unique_ptr<Mesh> ( loadMesh ( device, "../../Models/teapot.3ds", 0.04f ) );

		sampler.create   ( device );		// use default optiona		
		texture.load     ( device, "textures/texture.jpg", false );
		createPipelines  ();

		textLayer.create  ( &device, 1024 );		// so that renderPass is already ready
		textLayer.addText ( "Hello, World !", 0, 0 );
	}

	void	createUniformBuffers ()
	{
		uniformBuffers.resize ( swapChain.imageCount() );
		
		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].create ( device, sizeof ( UniformBufferObject ), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
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
				.addUniformBuffer ( 0, uniformBuffers [i], 0, sizeof ( UniformBufferObject ) )
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

		VkFence			currentFence  = swapChain.currentInFlightFence ();

		vkResetFences ( device.getDevice (), 1, &currentFence );

		SubmitInfo ()
			.wait    ( { { swapChain.currentAvailableSemaphore (), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT } } )
			.buffers ( { commandBuffers [imageIndex] } )
			.signal  ( { textLayer.getSemaphore () } )
			.submit  ( device.getGraphicsQueue (), swapChain.currentInFlightFence () );

		SubmitInfo ()
			.wait    ( { { textLayer.getSemaphore ().getHandle (), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT } } )
			.buffers ( { textLayer.getCommandBuffer ( imageIndex ) } )
			.signal  ( { swapChain.currentRenderFinishedSemaphore () } )
			.submit  ( device.getGraphicsQueue () );

	}

	void	createCommandBuffers ( Renderpass& renderPass )
	{
		auto	framebuffers = swapChain.getFramebuffers ();

		commandBuffers = device.allocCommandBuffers ( (uint32_t)framebuffers.size ());

		for ( size_t i = 0; i < commandBuffers.size(); i++ )
		{
			commandBuffers [i]
				.begin             ()
				.beginRenderPass   ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).clearColor ( 0, 0, 0, 1 ).clearDepthStencil ().extent ( swapChain.getExtent ().width, swapChain.getExtent ().height ) )
				.pipeline          ( pipeline )
				.addDescriptorSets ( { descriptorSets[i] } )
				.render            ( mesh.get () )
				.end               ();
		}
	}

	void updateUniformBuffer ( uint32_t currentImage )
	{
		float				time = (float)getTime ();
		UniformBufferObject ubo  = {};
		auto				mv   = controller->getModelView  ();
		auto				proj = controller->getProjection ();

		ubo.model = mv;
		ubo.view  = glm::mat4 ( 1 );
		ubo.proj  = proj;

		uniformBuffers [currentImage].copy ( &ubo, sizeof ( ubo ) );
	}
};

int main ( int argc, const char * argv [] ) 
{
	return ExampleWindow ( 800, 600, "Test window" ).run ();
}
