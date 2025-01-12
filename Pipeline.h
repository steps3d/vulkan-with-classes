
#pragma once

#include	<array>
#include	<memory>			// for shared_ptr
#include	"Data.h"
#include	"Texture.h"

class	GraphicsPipeline;

// function to register all vertex attributes by a vertex type
template <typename T>
inline GraphicsPipeline&	registerVertexAttrs ( GraphicsPipeline& pipeline ) 
{
	return pipeline;
}

class	Shader 
{
	VkShaderModule	shader = VK_NULL_HANDLE;
	VkDevice		device = VK_NULL_HANDLE;
	std::string		name   = "main";
	
public:
	Shader  () {}
	Shader ( Shader&& sh )
	{
		std::swap ( device, sh.device );
		std::swap ( shader, sh.shader );
		std::swap ( name,   sh.name   );
	}
	Shader ( const Shader& ) = delete;
	~Shader () 
	{
		clean ();
	}

	Shader& operator = ( const Shader& ) = delete;

	VkShaderModule	getHandle () const
	{
		return shader;
	}

	const char * getName () const	// get entry point
	{
		return name.c_str ();
	}
	
	void	clean ()
	{
		if ( shader != VK_NULL_HANDLE )
			vkDestroyShaderModule ( device, shader, nullptr );
		
		shader = VK_NULL_HANDLE;
	}

	void	load ( VkDevice dev, Data& data )
	{
		VkShaderModuleCreateInfo createInfo = {};

		device              = dev;
		createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = data.getLength ();
		createInfo.pCode    = reinterpret_cast<const uint32_t*>( data.getPtr () );

		if ( vkCreateShaderModule ( device, &createInfo, nullptr, &shader ) != VK_SUCCESS )
			fatal () << "Failed to create shader module! ";
	}

	void	setName ( const char * nm )
	{
		name = nm;
	}
};

class	Renderpass
{
	VkDevice								device     = VK_NULL_HANDLE;
	VkRenderPass 							renderPass = VK_NULL_HANDLE;
	std::vector<VkAttachmentDescription>	attachments;
	std::vector<VkAttachmentReference>		subpasses;
	VkAttachmentReference					depthRef  = {};		// depth attachment is separate
	bool									hasDepth  = false;
	
public:
	Renderpass () {}
	Renderpass ( Renderpass&& p )
	{
		std::swap ( device,      p.device      );
		std::swap ( renderPass,  p.renderPass  );
		std::swap ( attachments, p.attachments );
		std::swap ( subpasses,   p.subpasses   );
	}

	Renderpass ( const Renderpass& ) = delete;
	~Renderpass ()
	{
		clean ();
	}

	VkRenderPass	getHandle () const
	{
		return renderPass;
	}
	
	void	clean ()
	{
		if ( renderPass != VK_NULL_HANDLE )
			vkDestroyRenderPass ( device, renderPass, nullptr );
		
		renderPass = VK_NULL_HANDLE;
		
		attachments.clear ();
		subpasses.clear   ();
	}
	
	Renderpass&	addAttachment ( VkFormat format, VkImageLayout initialLayout, 
							VkImageLayout finalLayout, 
							VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, 
							VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE, 
							VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, 
							VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE )
	{
		VkAttachmentDescription colorAttachment = {};
		
		colorAttachment.format         = format;
		colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp         = loadOp;
		colorAttachment.storeOp        = storeOp;
		colorAttachment.stencilLoadOp  = stencilLoadOp;
		colorAttachment.stencilStoreOp = stencilStoreOp;
		colorAttachment.initialLayout  = initialLayout;
		colorAttachment.finalLayout    = finalLayout;
		
		attachments.push_back ( colorAttachment );
		
		return *this;
	}
	
	Renderpass&	addAttachment ( VkAttachmentDescription& desc )
	{
		attachments.push_back ( desc );
		
		return *this;
	}
	
	Renderpass&	addSubpass ( uint32_t attachment, 
							 VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
	{
		VkAttachmentReference	colorAttachmentRef = {};
		
		colorAttachmentRef.attachment = attachment;
		colorAttachmentRef.layout     = layout;	
		
		subpasses.push_back ( colorAttachmentRef );
		
		return *this;
	}
	
	Renderpass&	addDepthSubpass ( uint32_t attachment, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		depthRef.attachment = attachment;
		depthRef.layout     = layout;	
		hasDepth            = true;
		
		return *this;
	}
	
	void	create ( Device& dev )
	{
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = (uint32_t)subpasses.size ();
		subpass.pColorAttachments    = subpasses.data ();

		if ( hasDepth )
			subpass.pDepthStencilAttachment = &depthRef;
		
		VkSubpassDependency dependency = {};
		
		dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass    = 0;
		dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};

		renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = (uint32_t)attachments.size ();
		renderPassInfo.pAttachments    = attachments.data ();
		renderPassInfo.subpassCount    = 1;
		renderPassInfo.pSubpasses      = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies   = &dependency;

		if ( vkCreateRenderPass ( device = dev.getDevice (), &renderPassInfo, nullptr, &renderPass ) != VK_SUCCESS )
			fatal () << "Renderpass: failed to create render pass!";
	}
	
	uint32_t	numColorAttachments () const
	{
		uint32_t	num = 0;
		
		for ( auto& at : attachments )
			if ( !isDepthFormat ( at.format ) )
				num ++;
			
		return num;
	}
	
	uint32_t	numDepthAttachments () const
	{
		uint32_t	num = 0;
		
		for ( auto& at : attachments )
			if ( isDepthFormat ( at.format ) )
				num ++;
			
		return num;
	}
};

class	BindingDescription
{
	std::vector<VkVertexInputBindingDescription> descr;

public:
	BindingDescription () = default;

	void	clean ()
	{
		descr.clear ();
	}

	BindingDescription& addBinding ( uint32_t stride, uint32_t binding = 0, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX )
	{
		VkVertexInputBindingDescription	bindingDescription = {};
		
		bindingDescription.binding   = binding;
		bindingDescription.stride    = stride;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		
		descr.push_back ( bindingDescription );

		return *this;
	}
	
	const VkVertexInputBindingDescription * data () const
	{
		return descr.data ();
	}
	
	uint32_t	count () const
	{
		return (uint32_t) descr.size ();
	}	
};

class	AttrDescription
{
	std::vector<VkVertexInputAttributeDescription>	descr;
	
public:
	AttrDescription () = default;
	
	uint32_t	count () const
	{
		return (uint32_t)descr.size ();
	}
	
	const VkVertexInputAttributeDescription * data () const
	{
		return descr.data ();
	}
	
	void	clean ()
	{
		descr.clear ();
	}

	AttrDescription&	addAttr ( uint32_t binding, uint32_t location, VkFormat format, uint32_t offset )
	{
		VkVertexInputAttributeDescription	attributeDescription;
		
		attributeDescription.binding  = binding;
		attributeDescription.location = location;
		attributeDescription.format   = format;
		attributeDescription.offset   = offset;
		
		descr.push_back ( attributeDescription );
		
		return *this;
	}
};

class	DescSetLayout
{
	std::vector<VkDescriptorSetLayoutBinding>	descr;
	std::vector<VkDescriptorBindingFlags>		flags;
	std::shared_ptr<bool>						shared;		// shared marker used to track shared usage
	VkDescriptorSetLayout						descriptorSetLayout = VK_NULL_HANDLE;
	VkDevice									device              = VK_NULL_HANDLE;

public:
	DescSetLayout () = default;
	DescSetLayout ( const DescSetLayout& dsl) : descr ( dsl.descr ), flags ( dsl.flags ), descriptorSetLayout ( dsl.descriptorSetLayout ), device ( dsl.device ) 
	{
		if ( descriptorSetLayout )		// share only if there is layout to share
			shared = dsl.shared;
	}

	DescSetLayout ( DescSetLayout&& dsl )
	{
		std::swap ( device,              dsl.device );	
		std::swap ( descriptorSetLayout, dsl.descriptorSetLayout );
		std::swap ( descr,               dsl.descr );
		std::swap ( shared,              dsl.shared );
	}
	
	~DescSetLayout ()
	{
		clean ();		// clean will check for sharing
	}
	
	DescSetLayout& operator = ( const DescSetLayout& dsl )
	{
		cleanLayout ();

		descr               = dsl.descr;
		flags               = dsl.flags;
		device              = dsl.device;
		descriptorSetLayout = dsl.descriptorSetLayout;

		if ( dsl.descriptorSetLayout )	// realy shared layout
			shared = dsl.shared;

		return *this;
	}

	void	operator = ( DescSetLayout&& dsl )
	{
		//assert ( descriptorSetLayout == VK_NULL_HANDLE );		// we should not have ready layout (or may be descroy it ?)
		
		std::swap ( device,              dsl.device );
		std::swap ( descriptorSetLayout, dsl.descriptorSetLayout );
		std::swap ( descr,               dsl.descr  );
		std::swap ( shared,              dsl.shared );
	}
	
	uint32_t	count () const
	{
		return (uint32_t)descr.size ();
	}
	
	const VkDescriptorSetLayoutBinding * data () const
	{
		return descr.data ();
	}

	VkDescriptorSetLayout	getHandle () const
	{
		return descriptorSetLayout;
	}
	
	void	clean ()
	{
		cleanLayout  ();
		descr.clear  ();
		shared.reset ();		// now we're not sharing with anyone
	}
	
	DescSetLayout&	add ( uint32_t binding, VkDescriptorType type, VkShaderStageFlags flags, uint32_t cnt = 1 )
	{
		assert ( descriptorSetLayout == VK_NULL_HANDLE );		// we should not have ready layout (or may be descroy it ?)
		assert ( cnt > 0 );

		VkDescriptorSetLayoutBinding	layoutBinding = {};
		
		layoutBinding.binding            = binding;
		layoutBinding.descriptorCount    = cnt;
		layoutBinding.descriptorType     = type;
		layoutBinding.pImmutableSamplers = nullptr;
		layoutBinding.stageFlags         = flags;
		
		descr.push_back ( layoutBinding );
		
		return *this;
	}
	
	DescSetLayout&	addFlags ( std::initializer_list<VkDescriptorBindingFlags> flagsList )
	{
		assert ( descriptorSetLayout == VK_NULL_HANDLE );		// we should not have ready layout (or may be descroy it ?)

		for ( auto f : flagsList )
			flags.push_back ( f );

		return *this;
	}

	void	create ( VkDevice dev, VkDescriptorSetLayoutCreateFlags  layoutCreateFlags = 0 )
	{
			// if already have value then clean it
		cleanLayout ();

		VkDescriptorSetLayoutCreateInfo 			layoutInfo            = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };

		if ( !flags.empty () )
		{
			assert ( flags.size () == descr.size () );

			setLayoutBindingFlags.bindingCount  = uint32_t ( flags.size () );
			setLayoutBindingFlags.pBindingFlags = flags.data ();

			layoutInfo.pNext = &setLayoutBindingFlags;
		}

		////////////
		for ( auto& it : descr)
			assert(it.descriptorCount > 0);

		///////////
		layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = count ();
		layoutInfo.pBindings    = data  ();
		layoutInfo.flags        = layoutCreateFlags;
		shared                  = std::make_shared<bool> ( true );

		if ( vkCreateDescriptorSetLayout ( device = dev, &layoutInfo, nullptr, &descriptorSetLayout ) != VK_SUCCESS )
			fatal () << "DescSetLayout: failed to create descriptor set layout!";
	}

protected:
		// perform freeeing layout if not shared,
		// dec shared count
	void	cleanLayout ()
	{
			// if already have value then clean it
		if ( descriptorSetLayout != VK_NULL_HANDLE )
		{
				// if not shared now
			if ( shared.use_count () == 1 )
				vkDestroyDescriptorSetLayout ( device, descriptorSetLayout, nullptr );

			descriptorSetLayout = VK_NULL_HANDLE;
		}

		shared.reset ();		// one share less
	}
};

class	Pipeline		// base for pipelines
{
	Device            * device         = nullptr;
	VkPipelineLayout 	pipelineLayout = VK_NULL_HANDLE;
	VkPipeline			pipeline       = VK_NULL_HANDLE;

public:
	Pipeline  () = default;
	~Pipeline ()
	{
		clean ();
	}

	bool	isOk () const
	{
		return device != nullptr && pipelineLayout != VK_NULL_HANDLE && pipeline != VK_NULL_HANDLE;
	}

	void	clean ()
	{		
		if ( pipeline != VK_NULL_HANDLE )
			vkDestroyPipeline       ( device->getDevice (), pipeline,       nullptr );

		if ( pipelineLayout != VK_NULL_HANDLE )
			vkDestroyPipelineLayout ( device->getDevice (), pipelineLayout, nullptr );
		
		pipeline       = VK_NULL_HANDLE;
		pipelineLayout = VK_NULL_HANDLE;
	}
};

class	GraphicsPipeline
{
	Device			  * device         = nullptr;
	VkPipelineLayout 	pipelineLayout = VK_NULL_HANDLE;
	VkPipeline			pipeline       = VK_NULL_HANDLE;

	Shader	vertShader;
	Shader	fragShader;
	Shader	geomShader;
	Shader	tessControlShader, tessEvalShader;

	VkPrimitiveTopology                        topology                = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
 	VkBool32                                   primitiveRestartEnable  = VK_FALSE;

	VkBool32                                   depthClampEnable        = VK_FALSE;
	VkBool32                                   rasterizerDiscardEnable = VK_FALSE;
	VkPolygonMode                              polygonMode             = VK_POLYGON_MODE_FILL;
	VkCullModeFlags                            cullMode                = VK_CULL_MODE_BACK_BIT;
	VkFrontFace                                frontFace               = VK_FRONT_FACE_CLOCKWISE;
	VkBool32                                   depthBiasEnable         = VK_FALSE;
	float                                      depthBiasConstantFactor = 0.0f;
	float                                      depthBiasClamp          = 0.0f;
	float                                      depthBiasSlopeFactor    = 0.0f;
	float                                      lineWidth               = 1.0f;

	float										minDepth = 0.0f;
	float										maxDepth = 1.0f;
	uint32_t									width    = 0;
	uint32_t									height   = 0;
	uint32_t									patchSize = 0;
	
			// depth-stencil 
	VkBool32									depthTestEnable        = VK_FALSE;
	VkBool32									depthWriteEnable       = VK_FALSE;
	VkCompareOp                                 depthCompareOp         = VK_COMPARE_OP_LESS;
	VkBool32									depthBoundsTestEnable  = VK_FALSE;
	VkBool32									stencilTestEnable      = VK_FALSE;
	VkStencilOpState							front;			// for stencil test only
	VkStencilOpState							back;			// for stencil test only
	float										minDepthBounds         = 0.0f;
	float										maxDepthBounds         = 1.0f;
	
	VkBool32                                    logicOpEnable           = VK_FALSE;
	VkLogicOp                                   logicOp                 = VK_LOGIC_OP_COPY;
//    uint32_t                                      attachmentCount;
//    const VkPipelineColorBlendAttachmentState*    pAttachments;
	float                                       blendConstants[4]       = { 0, 0, 0, 0 };

				// VkPipelineColorBlendAttachmentState 
	VkBool32									blendEnable         = VK_FALSE;
	VkBlendFactor								srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	VkBlendFactor								dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	VkBlendOp									colorBlendOp        = VK_BLEND_OP_ADD;
	VkBlendFactor								srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	VkBlendFactor								dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	VkBlendOp									alphaBlendOp        = VK_BLEND_OP_ADD;
	VkColorComponentFlags						colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
													// data for vertex
	BindingDescription							vertexBindings;
	AttrDescription								vertexAttrs;
	std::vector<DescSetLayout>					descLayouts;
	std::vector<VkPushConstantRange>			pushConsts;
	void									  * pNext = nullptr;		// additional info for creating

	uint32_t									numColorBlendAttachments = 0;		// for dynamic rendering, when we don't have any valid renderpass

public:
	GraphicsPipeline () {}
	~GraphicsPipeline () 
	{
		clean ();
	}

	void	clean ()
	{		
		if ( !device )
			return;
		
		vkDestroyPipeline       ( device->getDevice (), pipeline,       nullptr );
		vkDestroyPipelineLayout ( device->getDevice (), pipelineLayout, nullptr );
		
		pipeline       = VK_NULL_HANDLE;
		pipelineLayout = VK_NULL_HANDLE;
		
		vertShader.clean     ();
		fragShader.clean     ();
		vertexBindings.clean ();
		vertexAttrs.clean    ();

		for ( auto& d : descLayouts )
			d.clean ();
	}
	
	VkPipeline	getHandle () const
	{
		return pipeline;
	}

	VkPipelineLayout	getLayout () const
	{
		return pipelineLayout;
	}

	const DescSetLayout&	getDescLayout ( int no = 0 ) const
	{
		assert ( no < descLayouts.size () );
		
		return descLayouts [no];
	}

	GraphicsPipeline&	setDevice ( Device& dev )
	{
		device = &dev;

		return *this;
	}

	GraphicsPipeline&	setVertexShader ( const std::string& fileName ) 
	{
		Data	data ( fileName );
		
		if ( data.getLength () < 1 )
			fatal () << "Shader: cannot open " << fileName << Log::endl;
		
		vertShader.load ( device->getDevice (), data );
		
		return *this; 
	}

	GraphicsPipeline&	setFragmentShader ( const std::string& fileName )
	{ 
		Data	data ( fileName );
		
		if ( data.getLength () < 1 )
			fatal () << "Shader: cannot open " << fileName << Log::endl;
		
		fragShader.load ( device->getDevice (), data );
		
		return *this; 
	}

	GraphicsPipeline&	setGeometryShader ( const std::string& fileName )
	{ 
		Data	data ( fileName );
		
		if ( data.getLength () < 1 )
			fatal () << "Shader: cannot open " << fileName << Log::endl;
		
		geomShader.load ( device->getDevice (), data );
		
		return *this; 
	}

	GraphicsPipeline&	setTessControlShader ( const std::string& fileName )
	{ 
		Data	data ( fileName );
		
		if ( data.getLength () < 1 )
			fatal () << "Shader: cannot open " << fileName << Log::endl;
		
		tessControlShader.load ( device->getDevice (), data );
		
		return *this; 
	}

	GraphicsPipeline&	setTessEvalShader ( const std::string& fileName )
	{ 
		Data	data ( fileName );
		
		if ( data.getLength () < 1 )
			fatal () << "Shader: cannot open " << fileName << Log::endl;
		
		tessEvalShader.load ( device->getDevice (), data );
		
		return *this; 
	}

	GraphicsPipeline&	setPatchSize ( uint32_t pSize )
	{
		patchSize = pSize;

		return *this;
	}

	GraphicsPipeline&	addVertexBinding ( uint32_t stride, uint32_t binding = 0, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX )
	{
		vertexBindings.addBinding ( stride, binding, inputRate );
		
		return *this;
	}
	
	GraphicsPipeline&	addVertexAttr ( uint32_t binding, uint32_t location, VkFormat format, uint32_t offset )
	{
		vertexAttrs.addAttr ( binding, location, format, offset );
		
		return *this;
	}

	template <typename T>
	GraphicsPipeline&	addVertexAttributes ()
	{
		return registerVertexAttrs<T> ( *this );
	}

	GraphicsPipeline&	setTopology ( VkPrimitiveTopology t )
	{
		topology = t;

		return *this;
	}

	GraphicsPipeline&	setPrimitiveRestartEnable ( bool flag )
	{
		primitiveRestartEnable = flag ? VK_TRUE : VK_FALSE;

		return *this;
	}

	GraphicsPipeline&	setDepthClampEnable ( bool flag = false )
	{
		depthClampEnable = flag ? VK_TRUE : VK_FALSE;

		return *this;
	}

	GraphicsPipeline&	setRasterizerDiscardEnable ( bool flag = false )
	{
		rasterizerDiscardEnable = flag ? VK_TRUE : VK_FALSE;

		return *this;
	}

	GraphicsPipeline&	setPolygonMode ( VkPolygonMode mode )
	{
		polygonMode = mode;

		return *this;
	}

	GraphicsPipeline&	setCullMode ( VkCullModeFlags flags )
	{
		cullMode = flags;

		return *this;
	}

	GraphicsPipeline&	setFrontFace ( VkFrontFace face )
	{
		frontFace = face;

		return *this;
	}

	GraphicsPipeline&	setDepthBiasEnable ( bool flag = false )
	{
		depthBiasEnable = flag ? VK_TRUE : VK_FALSE;

		return *this;
	}

	GraphicsPipeline&	setDepthBiasConstantFactor ( float factor )
	{
		depthBiasConstantFactor = factor;

		return *this;
	}

	GraphicsPipeline&	setDepthBiasClamp ( float v )
	{
		depthBiasClamp = v;

		return *this;
	}

	GraphicsPipeline&	setDepthBiasSlopeFactor ( float factor )
	{
		depthBiasSlopeFactor = factor;

		return *this;
	}

	GraphicsPipeline&	setLineWidth ( float width )
	{
		lineWidth = width;

		return *this;
	}

	GraphicsPipeline&	setSize ( uint32_t w, uint32_t h )	// swapExtent.width/height
	{
		width  = w;
		height = h;

		return *this;
	}

	GraphicsPipeline&	setSize ( const VkExtent2D& extent )	// swapExtent.width/height
	{
		width  = extent.width;
		height = extent.height;

		return *this;
	}

	GraphicsPipeline&	setDepthTest ( bool flag )
	{
		depthTestEnable = flag ? VK_TRUE : VK_FALSE;

		return *this;
	}
	
	GraphicsPipeline&	setDepthWrite ( bool flag )
	{
		depthWriteEnable = flag ? VK_TRUE : VK_FALSE;

		return *this;
	}
	
	GraphicsPipeline&	setDepthCompareOp ( VkCompareOp op )
	{
		depthCompareOp = op;

		return *this;
	}
	
	GraphicsPipeline&	setDepthBoundsTest ( bool flag, float minV, float maxV )
	{
		depthBoundsTestEnable = flag;
		minDepthBounds        = minV;
		maxDepthBounds        = maxV;

		return *this;
	}
	
	GraphicsPipeline&	setStencilTest ( bool flag )
	{
		stencilTestEnable = flag ? VK_TRUE : VK_FALSE;

		return *this;
	}
	
	GraphicsPipeline&	setStencilOp ( VkStencilOpState op )
	{
		front = op;
		back  = op;

		return *this;
	}
	
	GraphicsPipeline&	setStencilOp ( VkStencilOpState f, VkStencilOpState b )
	 {
		front = f;
		back  = b;

		return *this;
	}
	
	GraphicsPipeline&	setLogicOpEnable ( bool flag )
	{
		logicOpEnable = flag ? VK_TRUE : VK_FALSE;

		return *this;
	}
	
	GraphicsPipeline&	setLogicOp ( VkLogicOp op ) 
	{
		logicOp = op;

		return *this;
	}

	GraphicsPipeline&	setMinDepth ( float depth )
	{
		minDepth = depth;

		return *this;
	}
	
	GraphicsPipeline&	setMaxDepth ( float depth )
	{
		maxDepth = depth;

		return *this;
	}
	
	GraphicsPipeline&	setBlendEnable ( bool flag )
	{
		blendEnable = flag ? VK_TRUE : VK_FALSE;
		
		return *this;
	}
	
	GraphicsPipeline&	setColorBlendFactors ( VkBlendFactor srcFactor, VkBlendFactor dstFactor )
	{
		srcColorBlendFactor = srcFactor;
		dstColorBlendFactor = dstFactor;
		
		return *this;
	}
	
	GraphicsPipeline&	setAlphaBlendFactors ( VkBlendFactor srcFactor, VkBlendFactor dstFactor )
	{
		srcAlphaBlendFactor = srcFactor;
		dstAlphaBlendFactor = dstFactor;
		
		return *this;
	}
	
	GraphicsPipeline&	setColorBlendOp ( VkBlendOp op )
	{
		colorBlendOp = op;
		
		return *this;
	}
	
	GraphicsPipeline&	setAlphaBlendOp ( VkBlendOp op )
	{
		alphaBlendOp = op;
		
		return *this;
	}
	
	GraphicsPipeline&	setColorWriteMask ( bool red, bool green, bool blue, bool alpha )
	{
		colorWriteMask = 0;
		
		if ( red )
			colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;

		if ( green )
			colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;

		if ( blue )
			colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;

		if ( alpha )
			colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

		return *this;
	}

		// for dynamic rendering we have to explcitly specify numColorBlendAttachments 
		// because we don't have valid renderpass
	GraphicsPipeline&	setNumColorBlendAttachments ( uint32_t num )
	{
		numColorBlendAttachments = num;

		return *this;
	}

	GraphicsPipeline&	addDescLayout ( uint32_t index, DescSetLayout&& dsl )
	{
		if ( index >= descLayouts.size () )
			descLayouts.resize ( index + 1 );
		
		std::swap ( descLayouts [index], dsl );			// XXX - move
		
		return *this;
	}
	
	GraphicsPipeline&	addDescLayout ( uint32_t index, DescSetLayout& dsl )
	{
		if ( index >= descLayouts.size () )
			descLayouts.resize ( index + 1 );
		
		descLayouts [index] = dsl;
		
		return *this;
	}
	
	GraphicsPipeline&	addPushConstRange ( VkShaderStageFlags stageFlags, uint32_t size, uint32_t offs = 0 )
	{
		pushConsts.push_back ( { stageFlags, offs, size } );

		return *this;
	}

	GraphicsPipeline&	addAddInfo ( void * info )
	{
		pNext = info;

		return *this;

	}
	void	create ( Renderpass& renderPass, uint32_t flags = 0 )
	{
		if ( !device )
			fatal () << "Pipeline: device is NULL" << Log::endl;
		
		VkPipelineShaderStageCreateInfo vertShaderStageInfo        = {};
		VkPipelineShaderStageCreateInfo fragShaderStageInfo        = {};
		VkPipelineShaderStageCreateInfo geomShaderStageInfo        = {};
		VkPipelineShaderStageCreateInfo tessControlShaderStageInfo = {};
		VkPipelineShaderStageCreateInfo tessEvalShaderStageInfo    = {};

		vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShader.getHandle ();
		vertShaderStageInfo.pName  = vertShader.getName   ();

		fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShader.getHandle ();
		fragShaderStageInfo.pName  = fragShader.getName   ();

		std::vector<VkPipelineShaderStageCreateInfo>	shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

				// optional - geometry shader
		if ( geomShader.getHandle () != VK_NULL_HANDLE )
		{
			geomShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			geomShaderStageInfo.stage  = VK_SHADER_STAGE_GEOMETRY_BIT;
			geomShaderStageInfo.module = geomShader.getHandle ();
			geomShaderStageInfo.pName  = geomShader.getName   ();

			shaderStages.push_back ( geomShaderStageInfo );
		}
		
			// optional - tessellation shaders
		if ( tessControlShader.getHandle () != VK_NULL_HANDLE )
		{
			tessControlShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			tessControlShaderStageInfo.stage  = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			tessControlShaderStageInfo.module = tessControlShader.getHandle ();
			tessControlShaderStageInfo.pName  = tessControlShader.getName   ();

			tessEvalShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			tessEvalShaderStageInfo.stage  = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			tessEvalShaderStageInfo.module = tessEvalShader.getHandle ();
			tessEvalShaderStageInfo.pName  = tessEvalShader.getName   ();

			shaderStages.push_back ( tessControlShaderStageInfo );
			shaderStages.push_back ( tessEvalShaderStageInfo    );
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};

		vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount   = vertexBindings.count ();
		vertexInputInfo.vertexAttributeDescriptionCount = vertexAttrs.count    ();
		
		if ( vertexBindings.count () > 0 )
			vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data ();
		
		if ( vertexAttrs.count () > 0 )
			vertexInputInfo.pVertexAttributeDescriptions = vertexAttrs.data ();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};

		inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology               = topology;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};

		viewport.x        = 0.0f;
		viewport.y        = 0.0f;
		viewport.width    = (float) width;
		viewport.height   = (float) height;
		viewport.minDepth = minDepth;
		viewport.maxDepth = maxDepth;

		VkRect2D scissor = {};
		
		scissor.offset = {0, 0};
		scissor.extent = VkExtent2D { width, height };

		VkPipelineViewportStateCreateInfo viewportState = {};

		viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports    = &viewport;
		viewportState.scissorCount  = 1;
		viewportState.pScissors     = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};

		rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable        = depthClampEnable;
		rasterizer.rasterizerDiscardEnable = rasterizerDiscardEnable;
		rasterizer.polygonMode             = polygonMode;
		rasterizer.lineWidth               = lineWidth;
		rasterizer.cullMode                = cullMode;
		rasterizer.frontFace               = frontFace;
		rasterizer.depthBiasEnable         = depthBiasEnable;
		rasterizer.depthBiasConstantFactor = depthBiasConstantFactor;
		rasterizer.depthBiasClamp          = depthBiasClamp;
		rasterizer.depthBiasSlopeFactor    = depthBiasSlopeFactor;

		VkPipelineMultisampleStateCreateInfo multisampling = {};

		multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable  = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		
		depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable       = depthTestEnable;
		depthStencil.depthWriteEnable      = depthWriteEnable;
		depthStencil.depthCompareOp        = depthCompareOp;
		depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
		depthStencil.stencilTestEnable     = stencilTestEnable;

		if ( depthBoundsTestEnable )
		{
			depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
			depthStencil.minDepthBounds        = minDepthBounds;
			depthStencil.maxDepthBounds        = maxDepthBounds;
		}
		
		if ( numColorBlendAttachments < 1 )
			numColorBlendAttachments = std::max<uint32_t> ( 1, renderPass.numColorAttachments () );

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;

		for ( uint32_t i = 0; i < numColorBlendAttachments; i++ )
		{
			VkPipelineColorBlendAttachmentState colorBlendAttachment = {};

			colorBlendAttachment.colorWriteMask = colorWriteMask;
			colorBlendAttachment.blendEnable    = blendEnable;

			if ( blendEnable )
			{
				colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
				colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
				colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
				colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
				colorBlendAttachment.colorBlendOp        = colorBlendOp;
				colorBlendAttachment.alphaBlendOp        = alphaBlendOp;
			}

			colorBlendAttachments.push_back ( colorBlendAttachment );
		}

		VkPipelineColorBlendStateCreateInfo colorBlending = {};

		colorBlending.sType                = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable        = VK_FALSE;
		colorBlending.logicOp              = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount      = numColorBlendAttachments;
		colorBlending.pAttachments         = colorBlendAttachments.data ();
		colorBlending.blendConstants[0]    = 0.0f;
		colorBlending.blendConstants[1]    = 0.0f;
		colorBlending.blendConstants[2]    = 0.0f;
		colorBlending.blendConstants[3]    = 0.0f;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		
		pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount         = 0;
		
				// now use all descLayouts for creating pipeline layout
		std::vector<VkDescriptorSetLayout>	layouts;

		if ( !descLayouts.empty () )
		{
			for ( auto& d : descLayouts )
			{
					// create descriptor if not already created
				if ( d.getHandle () == VK_NULL_HANDLE )
					d.create ( device->getDevice () );

				layouts.push_back ( d.getHandle () );
			}
		
			pipelineLayoutInfo.setLayoutCount = (uint32_t) layouts.size ();
			pipelineLayoutInfo.pSetLayouts    = layouts.data ();
		}
		
		if ( !pushConsts.empty () )
		{
			pipelineLayoutInfo.pushConstantRangeCount = (int32_t) pushConsts.size ();
			pipelineLayoutInfo.pPushConstantRanges    = pushConsts.data ();
		}

		if ( vkCreatePipelineLayout ( device->getDevice (), &pipelineLayoutInfo, nullptr, &pipelineLayout ) != VK_SUCCESS )
			fatal () << "Pipeline: failed to create pipeline layout!" << std::endl;

		// Specify that these states will be dynamic, i.e. not part of pipeline state object.
		std::array<VkDynamicState, 2> dynamics { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamic = {};
		
		dynamic.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic.pDynamicStates    = dynamics.data ();
		dynamic.dynamicStateCount = uint32_t ( dynamics.size() );

		VkPipelineTessellationStateCreateInfo	tessStateCreateInfo = {};
		VkGraphicsPipelineCreateInfo			pipelineInfo        = {};
		
		pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount          = (uint32_t) shaderStages.size ();
		pipelineInfo.pStages             = shaderStages.data ();
		pipelineInfo.pVertexInputState   = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState      = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState   = &multisampling;
		pipelineInfo.pDepthStencilState  = &depthStencil;
		pipelineInfo.pColorBlendState    = &colorBlending;
		pipelineInfo.pDynamicState       = &dynamic;
		pipelineInfo.layout              = pipelineLayout;
		pipelineInfo.renderPass          = renderPass.getHandle ();
		pipelineInfo.subpass             = 0;
		pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
		pipelineInfo.flags               = flags;
		pipelineInfo.pNext               = pNext;

		if ( patchSize > 0 )
		{
			tessStateCreateInfo.patchControlPoints = patchSize;
			tessStateCreateInfo.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

			pipelineInfo.pTessellationState = &tessStateCreateInfo;
		}

		if ( vkCreateGraphicsPipelines ( device->getDevice (), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline ) != VK_SUCCESS )
			fatal () << "Pipeline: failed to create graphics pipeline!" << std::endl;
	}
};

class	ComputePipeline
{
	Device                            * device         = nullptr;
	VkPipelineLayout 					pipelineLayout = VK_NULL_HANDLE;
	VkPipeline							pipeline       = VK_NULL_HANDLE;
	Shader								shader;
	DescSetLayout						descLayout;
	std::vector<VkPushConstantRange>	pushConsts;

public:
	ComputePipeline  () = default;
	~ComputePipeline ()
	{
		clean ();
	}
	
	void	clean ()
	{		
		if ( pipeline != VK_NULL_HANDLE )
			vkDestroyPipeline       ( device->getDevice (), pipeline,       nullptr );

		if ( pipelineLayout != VK_NULL_HANDLE )
			vkDestroyPipelineLayout ( device->getDevice (), pipelineLayout, nullptr );
		
		pipeline       = VK_NULL_HANDLE;
		pipelineLayout = VK_NULL_HANDLE;
		
		shader.clean     ();
		descLayout.clean ();
	}
	
	VkPipeline	getHandle () const
	{
		return pipeline;
	}

	VkPipelineLayout	getLayout () const
	{
		return pipelineLayout;
	}

	const DescSetLayout&	getDescLayout () const
	{
		return descLayout;
	}
	
	ComputePipeline&	setDevice ( Device& dev )
	{
		device = &dev;

		return *this;
	}
	
	ComputePipeline&	setShader ( const std::string& fileName ) 
	{
		Data	data ( fileName );
		
		if ( data.getLength () < 1 )
			fatal () << "Shader: cannot open " << fileName << Log::endl;
		
		shader.load ( device->getDevice (), data );
		
		return *this; 
	}

	ComputePipeline&	addDescriptor ( uint32_t binding, VkDescriptorType type, VkShaderStageFlags flags, uint32_t cnt = 1 )
	{
		descLayout.add ( binding, type, flags, cnt );

		return *this;
	}

	ComputePipeline&	addPushConstRange ( VkShaderStageFlags stageFlags, uint32_t size, uint32_t offs = 0 )
	{
		pushConsts.push_back ( { stageFlags, offs, size } );

		return *this;
	}


	ComputePipeline&	create ( uint32_t flags = 0 )
	{
		if ( !device )
			fatal () << "Pipeline: device is NULL" << Log::endl;
		
		VkComputePipelineCreateInfo		pipelineInfo       = {};
		VkPipelineShaderStageCreateInfo stageInfo          = {};	
		VkPipelineLayoutCreateInfo		pipelineLayoutInfo = {};
		
		pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		
		if ( descLayout.count () > 0 )
		{
			descLayout.create ( device->getDevice () );
			
			VkDescriptorSetLayout	descriptorSetLayout = descLayout.getHandle ();
			
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts    = &descriptorSetLayout;
		}

		if ( !pushConsts.empty () )
		{
			pipelineLayoutInfo.pushConstantRangeCount = (int32_t) pushConsts.size ();
			pipelineLayoutInfo.pPushConstantRanges    = pushConsts.data ();
		}

		if ( vkCreatePipelineLayout ( device->getDevice (), &pipelineLayoutInfo, nullptr, &pipelineLayout ) != VK_SUCCESS )
			fatal () << "Pipeline: failed to create pipeline layout!" << std::endl;

		stageInfo.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage     = VK_SHADER_STAGE_COMPUTE_BIT;
		stageInfo.module    = shader.getHandle ();
		stageInfo.pName     = shader.getName ();
		
		pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage  = stageInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.flags  = flags;
		
		if ( vkCreateComputePipelines ( device->getDevice (), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline ) != VK_SUCCESS )
			fatal () << "Pipeline: failed to create compute pipeline!" << std::endl;

		return *this;
	}
};
