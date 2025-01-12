#include	<ctype.h>
#include	<memory>
#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Mesh.h"
#include	"Controller.h"

struct UboCamera 
{
	glm::mat4 proj;
	glm::mat4 view;
};

struct UboModel
{
	glm::mat4 model;
};

class	ExampleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	GraphicsPipeline				pipeline;
	Renderpass						renderPass;
	std::vector<Texture>			textures;
	Sampler							sampler;
	std::unique_ptr<Mesh>           mesh;
	Uniform<UboCamera>				cameraUbo;					// camera uniform buffer
	std::vector<Uniform<UboModel>>	modelUbos;					// models uniform buffers

	DescSetLayout					descSetLayoutBuffers;		// descriptor set layout for descriptor buffer with buffers (Ubo)
	DescSetLayout					descSetLayoutTextures;		// descriptor set layout for descriptor buffer with textures
	PersistentBuffer				buffersDescriptorBuffer;	// descriptor buffer with buffers (Ubo)
	VkDeviceSize					buffersDescriptorOffset;
	VkDeviceSize					buffersDescriptorSize;
	PersistentBuffer				texturesDescriptorBuffer;	// descriptor buffer with textures
	VkDeviceSize					texturesDescriptorOffset;
	VkDeviceSize					texturesDescriptorSize;
																// we need properties for buffer alignment
	VkPhysicalDeviceDescriptorBufferPropertiesEXT& descriptorBufferProperties;

																// function pointers to new commands
	PFN_vkGetDescriptorSetLayoutSizeEXT vkGetDescriptorSetLayoutSizeEXT                           = {};
	PFN_vkGetDescriptorSetLayoutBindingOffsetEXT vkGetDescriptorSetLayoutBindingOffsetEXT         = {};
	PFN_vkCmdBindDescriptorBuffersEXT vkCmdBindDescriptorBuffersEXT                               = {};
	PFN_vkCmdSetDescriptorBufferOffsetsEXT vkCmdSetDescriptorBufferOffsetsEXT                     = {};
	PFN_vkGetDescriptorEXT vkGetDescriptorEXT                                                     = {};
	PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT vkCmdBindDescriptorBufferEmbeddedSamplersEXT = {};

public:
	ExampleWindow ( int w, int h, const std::string& t, bool depth, DevicePolicy * p, VkPhysicalDeviceDescriptorBufferPropertiesEXT& descBufferProps ) : VulkanWindow ( w, h, t, depth, p ), descriptorBufferProperties ( descBufferProps )
	{
		loadExtensions ();
		setController  ( new RotateController ( this, glm::vec3(2.0f, 2.0f, 2.0f) ) );

		mesh = std::unique_ptr<Mesh> ( createKnot ( device, 0.2f, 0.13f, 120, 30 ) );

		createDescriptorSets ();				// create descriptor sets for descriptor buffers

			// create buffers
		createDescriptorBuffer ( buffersDescriptorBuffer,  descSetLayoutBuffers,  true,  4, 0, buffersDescriptorSize,  buffersDescriptorOffset  );
		createDescriptorBuffer ( texturesDescriptorBuffer, descSetLayoutTextures, false, 3, 0, texturesDescriptorSize, texturesDescriptorOffset );

		sampler.create       ( device );		// use default options
		createTextures       ();
		createPipelines      ();
		uploadBuffer         ( cameraUbo, 0, 0 );

		for ( int i = 0; i <  modelUbos.size (); i++ )
		{
			uploadBuffer ( modelUbos [i], i + 1, 0 );
			uploadImage  ( textures [i], sampler, i );
		}
	}

	void	createTextures ()
	{
		std::string					texPath = "../../Textures/";
		std::vector<const char *>	texs    = { "Fieldstone.dds", "16.jpg", "flower.png", "lena.png", "block.jpg", "brick.tga", "oak.jpg", "rockwall.jpg" };

		textures.resize ( texs.size () );
		for ( size_t i = 0; i < texs.size (); i++ )
			if ( !textures [i].load ( device, texPath + texs [i] ) )
				fatal () << "Error loading texture " << texs [i] << std::endl;
	}

	void	createDescriptorBuffer ( PersistentBuffer& buf, const DescSetLayout& descLayout, bool isBuffer, int sizeMultiplier, uint32_t binding, VkDeviceSize& size, VkDeviceSize& offset )
	{
			// get size & offset
		vkGetDescriptorSetLayoutSizeEXT          ( device.getDevice (), descLayout.getHandle (), &size );
		vkGetDescriptorSetLayoutBindingOffsetEXT ( device.getDevice (), descLayout.getHandle (), binding, &offset );

			// force size alignment
		VkDeviceSize allocSize = sizeMultiplier * alignedSize<VkDeviceSize> ( size, descriptorBufferProperties.descriptorBufferOffsetAlignment );

		uint32_t	usage;

		if ( isBuffer )
			usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		else
			usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;

		buf.create ( device, allocSize, usage, Buffer::hostWrite );
	}

		// set entry index in buffers descriptor buffer to given buffer and offset in it
	void	uploadBuffer ( Buffer& buffer, uint32_t index, VkDeviceSize offset = 0 )
	{
		VkDescriptorGetInfoEXT		descriptorInfo        = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT     };
		VkDescriptorAddressInfoEXT	descriptorAddressInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
		char*						descriptorBufPtr      = (char*)buffersDescriptorBuffer.getPtr ();

		descriptorAddressInfo.address = buffer.getDeviceAddress () + offset;
		descriptorAddressInfo.range   = buffer.getSize ();		//buffersDescriptorSize;
		descriptorAddressInfo.format  = VK_FORMAT_UNDEFINED;

		descriptorInfo.type                       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorInfo.data.pCombinedImageSampler = nullptr;
		descriptorInfo.data.pUniformBuffer        = &descriptorAddressInfo;

		vkGetDescriptorEXT ( device.getDevice (), &descriptorInfo, descriptorBufferProperties.uniformBufferDescriptorSize, descriptorBufPtr + index * buffersDescriptorSize + buffersDescriptorOffset );
	}

	void	uploadImage ( Texture& texture, Sampler& sampler, uint32_t index, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
	{
		VkDescriptorGetInfoEXT	descriptorInfo   = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
		VkDescriptorImageInfo	imageDescriptor  = {};
		char*					descriptorBufPtr = (char*)texturesDescriptorBuffer.getPtr ();

		imageDescriptor.sampler     = sampler.getHandle    ();
		imageDescriptor.imageView   = texture.getImageView ();
		imageDescriptor.imageLayout = imageLayout;

		descriptorInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorInfo.data.pCombinedImageSampler = &imageDescriptor;

		vkGetDescriptorEXT ( device.getDevice (), &descriptorInfo, descriptorBufferProperties.combinedImageSamplerDescriptorSize, descriptorBufPtr + index * texturesDescriptorSize + texturesDescriptorOffset );
	}

	void	createUniformBuffers ()
	{
		cameraUbo.create ( device );
		modelUbos.resize ( 3 );
		
		for ( size_t i = 0; i < 3; i++ )
			modelUbos [i].create ( device );
	}

	void	freeUniformBuffers ()
	{
		cameraUbo.clean ();
		modelUbos.clear ();
	}

	void	createDescriptorSets ()
	{
		descSetLayoutBuffers .add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT   ).create ( device.getDevice (), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT );
		descSetLayoutTextures.add ( 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT ).create ( device.getDevice (), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT );
	}
	
	virtual	void	createPipelines () override 
	{
		createUniformBuffers    ();
		createDefaultRenderPass ( renderPass );

		pipeline.setDevice ( device )
			.setVertexShader   ( "shaders/shader-descriptor-buffer.vert.spv" )
			.setFragmentShader ( "shaders/shader-descriptor-buffer.frag.spv" )
			.setSize           ( swapChain.getExtent () )
			.addVertexBinding  ( sizeof ( BasicVertex ) )
			.addVertexAttributes <BasicVertex> ()
			.addDescLayout ( 0, descSetLayoutBuffers  )
			.addDescLayout ( 1, descSetLayoutBuffers  )
			.addDescLayout ( 2, descSetLayoutTextures )
			.setCullMode       ( VK_CULL_MODE_NONE  )
			.setDepthTest      ( true )
			.setDepthWrite     ( true )
			.create            ( renderPass, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT );			

				// create before command buffers
		swapChain.createFramebuffers ( renderPass, depthTexture.getImageView () );

		createCommandBuffers ( renderPass );
	}

	virtual	void	freePipelines () override
	{
		commandBuffers.clear ();
		pipeline.clean       ();
		renderPass.clean     ();
		freeUniformBuffers   ();
		descAllocator.clean  ();
	}
	
	virtual	void	submit ( uint32_t imageIndex ) override 
	{
		updateUniformBuffers ( imageIndex );
		defaultSubmit        ( commandBuffers [imageIndex] );
	}

	void	createCommandBuffers ( Renderpass& renderPass )
	{
		auto	framebuffers = swapChain.getFramebuffers ();
		VkDescriptorBufferBindingInfoEXT	bindings [2] {};

		bindings [0].sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
		bindings [0].address = buffersDescriptorBuffer.getDeviceAddress ();
		bindings [0].usage   = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;	
																				   
		bindings [1].sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
		bindings [1].address = texturesDescriptorBuffer.getDeviceAddress ();
		bindings [1].usage   = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

		commandBuffers = device.allocCommandBuffers ( (uint32_t)framebuffers.size ());

		for ( size_t i = 0; i < commandBuffers.size(); i++ )
		{
			commandBuffers [i]
				.begin             ()
				.beginRenderPass   ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( swapChain.getExtent ().width, swapChain.getExtent ().height ).clearColor (0,0,0,1).clearDepthStencil () )
				.pipeline          ( pipeline )
				.setViewport       ( swapChain.getExtent () )
				.setScissor        ( swapChain.getExtent () );

				VkDeviceSize	bufferOffset     = 0;
				uint32_t		bufferIndexUbo   = 0;
				uint32_t		bufferIndexImage = 1;

					// set descriptor buffer bindings to our descriptor buffers
				vkCmdBindDescriptorBuffersEXT ( commandBuffers [i].getHandle (), 2, bindings );

					// camera ubo (set 0)
				vkCmdSetDescriptorBufferOffsetsEXT ( commandBuffers [i].getHandle (), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout (), 0, 1, &bufferIndexUbo, &bufferOffset );

				for ( int j = 0; j < 3; j++ )
				{
						// model ubo (set 1)
					bufferOffset = (j + 1) * buffersDescriptorSize;
					vkCmdSetDescriptorBufferOffsetsEXT ( commandBuffers [i].getHandle (), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout (), 1, 1, &bufferIndexUbo, &bufferOffset );

						// texture (set 2)
					bufferOffset = j * texturesDescriptorSize;
					vkCmdSetDescriptorBufferOffsetsEXT ( commandBuffers [i].getHandle (), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout (), 2, 1, &bufferIndexImage, &bufferOffset );
				
						// render mesh
					commandBuffers [i].render ( mesh.get () );
				}

				commandBuffers [i].end ();
		}
	}

	void updateUniformBuffers ( uint32_t )
	{
		float	t     = float ( getTime () );

		cameraUbo->view  = controller->getModelView ();
		cameraUbo->proj  = controller->getProjection ();

		modelUbos [0]->model = glm::translate ( glm::mat4(1), glm::vec3 ( -1, -1, 0 ) ) * glm::rotate ( glm::mat4(1), t, glm::vec3 ( 1, 0, 0 ) );
		modelUbos [1]->model = glm::translate ( glm::mat4(1), glm::vec3 ( 0, 0, 1 ) )   * glm::rotate ( glm::mat4(1), t, glm::vec3 ( 0, 1, 0 ) );
		modelUbos [2]->model = glm::translate ( glm::mat4(1), glm::vec3 ( 1, 0, 0 ) )   * glm::rotate ( glm::mat4(1), t, glm::vec3 ( 0, 1, 1 ) );
	}

	void	loadExtensions ()
	{
		vkGetDescriptorSetLayoutSizeEXT              = reinterpret_cast<PFN_vkGetDescriptorSetLayoutSizeEXT>             (vkGetDeviceProcAddr(device.getDevice (), "vkGetDescriptorSetLayoutSizeEXT"));
		vkGetDescriptorSetLayoutBindingOffsetEXT     = reinterpret_cast<PFN_vkGetDescriptorSetLayoutBindingOffsetEXT>    (vkGetDeviceProcAddr(device.getDevice (), "vkGetDescriptorSetLayoutBindingOffsetEXT"));
		vkCmdBindDescriptorBuffersEXT                = reinterpret_cast<PFN_vkCmdBindDescriptorBuffersEXT>               (vkGetDeviceProcAddr(device.getDevice (), "vkCmdBindDescriptorBuffersEXT"));
		vkGetDescriptorEXT                           = reinterpret_cast<PFN_vkGetDescriptorEXT>                          (vkGetDeviceProcAddr(device.getDevice (), "vkGetDescriptorEXT"));
		vkCmdBindDescriptorBufferEmbeddedSamplersEXT = reinterpret_cast<PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT>(vkGetDeviceProcAddr(device.getDevice (), "vkCmdBindDescriptorBufferEmbeddedSamplersEXT"));
		vkCmdSetDescriptorBufferOffsetsEXT           = reinterpret_cast<PFN_vkCmdSetDescriptorBufferOffsetsEXT>          (vkGetDeviceProcAddr(device.getDevice (), "vkCmdSetDescriptorBufferOffsetsEXT"));
	}
};

int main ( int argc, const char * argv [] ) 
{
	DevicePolicy	policy;

		// we require VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME device extension
	policy.addDeviceExtension ( VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME );

	VkPhysicalDeviceDescriptorBufferFeaturesEXT   descriptorBufferFeatures     = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT   };
	VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferProperties   = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT };

	descriptorBufferFeatures.descriptorBuffer = VK_TRUE;

	policy.addFeatures   ( &descriptorBufferFeatures    );
	policy.addProperties ( &descriptorBufferProperties  );

	return ExampleWindow ( 800, 600, "Vulkan descriptor buffer", true,  &policy, descriptorBufferProperties ).run ();
}
