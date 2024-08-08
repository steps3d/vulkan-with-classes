#include	"VulkanWindow.h"
#include	"Buffer.h"
#include	"DescriptorSet.h"
#include	"Semaphore.h"
#include	"CommandPool.h"
#include	"Controller.h"

struct Ubo 		// for render pipeline
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct  ParticleVertex
{
	glm::vec4	pos;
};

template <>
inline GraphicsPipeline&	registerVertexAttrs<ParticleVertex> ( GraphicsPipeline& pipeline ) 
{
	return pipeline
		.addVertexAttr ( 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ParticleVertex, pos) );		// binding, location, format, offset
}

class	ParticleWindow : public VulkanWindow
{
	std::vector<CommandBuffer>		commandBuffers;
	GraphicsPipeline				graphicsPipeline;
	ComputePipeline					computePipeline;
	Renderpass						renderPass;
	std::vector<Uniform<Ubo>>		uniformBuffers;
	std::vector<DescriptorSet> 		descriptorSets;
	Buffer							posBuffer;
	Buffer							velBuffer;
	DescriptorSet					computeDescriptorSet;
	CommandBuffer					computeCommandBuffer;
	Semaphore						computeSemaphore, graphicsSemaphore;
	CommandPool						computeCommandPool;
	std::vector<Fence>				fences;
	size_t							n;
	size_t							numParticles;
	float							t     = 0;				// current time in seconds
	float							zNear = 0.1f;
	float							zFar  = 100.0f;	

public:
	ParticleWindow ( int w, int h, const std::string& t ) : VulkanWindow ( w, h, t, true )
	{
		setController ( new RotateController ( this, glm::vec3(12.0f, 12.0f, 12.0f) ) );

		computeCommandPool.create ( device, false, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );
		graphicsSemaphore.create  ( device );
		computeSemaphore.create   ( device );
		computeSemaphore.signal   ( device.getGraphicsQueue () );
		
		initParticles   ( 32 );
		createPipelines ();

		graphicsSemaphore.signal ( device.getGraphicsQueue () );
	}

	void	createDescriptorSets ()
	{
		descriptorSets.resize ( swapChain.imageCount () );

		for ( uint32_t i = 0; i < swapChain.imageCount (); i++ )
			descriptorSets  [i]
				.setLayout        ( device, descAllocator, graphicsPipeline.getDescLayout () )
				.addUniformBuffer ( 0, uniformBuffers [i], 0, sizeof ( Ubo ) )
				.create           ();
		
		computeDescriptorSet
			.setLayout        ( device, descAllocator, computePipeline.getDescLayout () )
			.addStorageBuffer ( 0, posBuffer )
			.addStorageBuffer ( 1, velBuffer )
			.create           ();
	}
	
	virtual	void	createPipelines () override 
	{
		VkDeviceSize bufferSize = sizeof ( Ubo );

		uniformBuffers.resize ( swapChain.imageCount() );

		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].create ( device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT );

			// current app code
		renderPass.addAttachment   ( swapChain.getFormat (),                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR )
			.addAttachment   ( depthTexture.getImage ().getFormat (), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
			.addSubpass      ( 0 )
			.addDepthSubpass ( 1 )
			.create          ( device );
		
		graphicsPipeline
			.setDevice         ( device )
			.setVertexShader   ( "shaders/particles-render.vert.spv" )
			.setFragmentShader ( "shaders/particles-render.frag.spv" )
			.setSize           ( swapChain.getExtent () )
			.addVertexBinding  ( sizeof ( ParticleVertex ) )
			.addVertexAttributes <ParticleVertex> ()
			.addDescLayout     ( 0, DescSetLayout ()
				.add ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ) )
			.setTopology       ( VK_PRIMITIVE_TOPOLOGY_POINT_LIST )
			.create            ( renderPass );
			
		computePipeline
			.setDevice     ( device )
			.setShader     ( "shaders/particles-compute.comp.spv" )
			.addDescriptor ( 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT )
			.addDescriptor ( 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT )
			.create        ();
		
				// create before command buffers
		swapChain.createFramebuffers ( renderPass, depthTexture.getImageView () );

		createDescriptorSets       ();
		createCommandBuffers       ( renderPass );
		createComputeCommandBuffer ();
		
		fences.resize ( swapChain.imageCount () );

		for ( auto& f : fences )
			f.create ( device );
	}

	virtual	void	freePipelines () override
	{
		for ( size_t i = 0; i < swapChain.imageCount (); i++ )
			uniformBuffers [i].clean ();

		commandBuffers.clear   ();
		graphicsPipeline.clean ();
		computePipeline.clean  ();
		renderPass.clean       ();
		descriptorSets.clear   ();
	}
	
	virtual	void	submit ( uint32_t imageIndex ) override 
	{
		updateUniformBuffer ( imageIndex );

		SubmitInfo ()
			.wait    ( { {graphicsSemaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT} } )
			.signal  ( { computeSemaphore } )
			.buffers ( { computeCommandBuffer } )
			.submit  ( device.getComputeQueue (), fences [currentImage].getHandle () );

		fences [currentImage].wait  ( UINT64_MAX );
		fences [currentImage].reset ();

		VkFence		currentFence  = swapChain.currentInFlightFence ();

		vkResetFences ( device.getDevice (), 1, &currentFence );

		SubmitInfo ()
			.wait    ( { { swapChain.currentAvailableSemaphore (), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { computeSemaphore.getHandle (), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT } } )
			.buffers ( { commandBuffers [imageIndex]  } )
			.signal  ( { swapChain.currentRenderFinishedSemaphore (), graphicsSemaphore.getHandle () } )
			.submit  ( device.getGraphicsQueue (), swapChain.currentInFlightFence () );
	}

	void	createCommandBuffers ( Renderpass& renderPass )
	{
		auto	framebuffers = swapChain.getFramebuffers ();

		commandBuffers = device.allocCommandBuffers ( (uint32_t)framebuffers.size ());

		for ( size_t i = 0; i < commandBuffers.size(); i++ )
		{
/*
			VkMemoryBarrier	memBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };

			memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			memBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

			vkCmdPipelineBarrier ( commandBuffers [i],
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				0, 1, &memBarrier,
				0, nullptr, 0, nullptr );
*/
			commandBuffers [i]
				.begin             ()
				.beginRenderPass   ( RenderPassInfo ( renderPass ).framebuffer ( framebuffers [i] ).extent ( swapChain.getExtent () ).clearColor ().clearDepthStencil () )
				.pipeline          ( graphicsPipeline )
				.addDescriptorSets ( { descriptorSets[i] } )
				.bindVertexBuffers ( { {posBuffer, 0}, { velBuffer, 1 } } )
				.draw              ( (uint32_t)numParticles, 1, 0, 0 )
				.end               ();
		}
	}

	void	createComputeCommandBuffer ()
	{
		computeCommandBuffer.create ( device );
		
		computeCommandBuffer.begin ();

			// Add memory barrier to ensure that the (graphics) vertex shader has fetched 
			// attributes before compute starts to write to the buffer
		if ( device.getGraphicsQueue () != device.getComputeQueue () )
		{
			transitionBuffer ( posBuffer, 0, VK_ACCESS_SHADER_WRITE_BIT, device.getGraphicsFamilyIndex (), device.getComputeFamilyIndex (), VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
			transitionBuffer ( velBuffer, 0, VK_ACCESS_SHADER_WRITE_BIT, device.getGraphicsFamilyIndex (), device.getComputeFamilyIndex (), VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
		}

		computeCommandBuffer
			.pipeline ( computePipeline )
			.addDescriptorSets ( { computeDescriptorSet } )
			.bindVertexBuffers ( { {posBuffer, 0 } } )
			.dispatch ( (uint32_t) (numParticles + 511) / 512, 1, 1 );

			// Add barrier to ensure that compute shader has finished writing to the buffer
			// Without this the (rendering) vertex shader may display incomplete results (partial data from last frame) 
		if ( device.getGraphicsQueue () != device.getComputeQueue () )
		{
			transitionBuffer ( posBuffer, VK_ACCESS_SHADER_WRITE_BIT, 0, device.getComputeFamilyIndex (), device.getGraphicsFamilyIndex (), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT );
			transitionBuffer ( velBuffer, VK_ACCESS_SHADER_WRITE_BIT, 0, device.getComputeFamilyIndex (), device.getGraphicsFamilyIndex (), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT );
		}

		computeCommandBuffer.end ();
	}

				// transition buffer from one queue family to another
	void	transitionBuffer ( Buffer& buf, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t srcFamily, uint32_t dstFamily, 
							   VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags )
	{
		VkBufferMemoryBarrier bufferBarrier =
		{
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			nullptr,
			srcAccessMask,
			dstAccessMask,
			srcFamily,
			dstFamily,
			posBuffer.getHandle (),
			0,
			VK_WHOLE_SIZE
		};

		vkCmdPipelineBarrier (
			computeCommandBuffer.getHandle (),
			srcStageFlags,
			dstStageFlags,
			0,
			0, nullptr,
			1, &bufferBarrier,
			0, nullptr );
	}
	
	void updateUniformBuffer ( uint32_t currentImage )
	{
		uniformBuffers [currentImage]->model = glm::mat4 ( 1.0f );
		uniformBuffers [currentImage]->view  = controller->getModelView ();
		uniformBuffers [currentImage]->proj  = controller->getProjection ();
	}
	
	void initParticles ( int num )
	{
		n            = num;
		numParticles = n * n * n;

				// init buffers with particle data
		std::vector<glm::vec4>	vb;
		std::vector<glm::vec4>	pb;
		float 					h = 2.0f / (n - 1);

		for ( size_t i = 0; i < n; i++ )
			for ( size_t j = 0; j < n; j++ )
				for ( size_t k = 0; k < n; k++ ) 
				{
					glm::vec4	p ( h * i - 1, h * j - 1, h * k - 1, 1 );

					pb.push_back ( p );
					vb.push_back ( glm::vec4 ( 0 ) );
				}

		posBuffer.create ( device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, pb, Buffer::hostWrite );
		velBuffer.create ( device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vb, Buffer::hostWrite );
	}
};

int main ( int argc, const char * argv [] ) 
{
	return ParticleWindow ( 1400, 900, "Compute particles" ).run ();
}
