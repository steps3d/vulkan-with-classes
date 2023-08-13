
#pragma once


class	Framebuffer
{
	struct	Attachment
	{
		Texture					texture;
		VkAttachmentDescription	description;
	};

	Device		  			  * device      = nullptr;
	VkFramebuffer				framebuffer = VK_NULL_HANDLE;
	uint32_t					width, height;	
	Renderpass					renderpass;
	std::vector<Attachment *>	attachments;

public:
	Framebuffer () = default;
	Framebuffer ( Framebuffer&& f )
	{
		std::swap ( device, f.device           );
		std::swap ( framebuffer, f.framebuffer );
		std::swap ( width,       f.width       );
		std::swap ( height,      f.height      );
		std::swap ( attachments, f.attachments );
		//renderpass = f.renderpass;
		//f.renderpass = Renderpass ()
	}
	Framebuffer ( const Framebuffer& ) = delete;
	~Framebuffer ()
	{
		clean ();
	}

	Framebuffer& operator = ( const Framebuffer& ) = delete;

	VkFramebuffer	getHandle () const
	{
		return framebuffer;
	}
	
	uint32_t	getWidth () const
	{
		return width;
	}
	
	uint32_t	getHeight () const
	{
		return height;
	}
	
	Renderpass&	getRenderpass ()
	{
		return renderpass;
	}
	
	const Texture&	getAttachment ( uint32_t index ) const
	{
		return attachments [index]->texture;
	}
	
	Texture&	getAttachment ( uint32_t index )
	{
		return attachments [index]->texture;
	}
	
	void		clean ()
	{
		for ( auto at : attachments )
			delete at;
		
		attachments.clear ();
		
		vkDestroyFramebuffer ( device->getDevice (), framebuffer, nullptr );
		
		framebuffer = VK_NULL_HANDLE;
	}

	Framebuffer&	init ( Device& dev, uint32_t w, uint32_t h )
	{
		device = &dev;
		width  = w;
		height = h;
		
		return *this;
	}
	
	Framebuffer&	addAttachment ( VkFormat format, VkImageUsageFlags usage, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED )
	{
		//VkImageAspectFlags	aspectMask = 0;
		VkImageLayout		imageLayout = finalLayout;

		if ( finalLayout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			if ( usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
			{
				//aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
				imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

			if ( usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) 
			{
				//aspectMask  = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
		}
	

		//assert ( aspectMask > 0 );

		Attachment * attachment = new Attachment;
		
		attachment->texture.create          ( *device, width, height, 1, 1, format, VK_IMAGE_TILING_OPTIMAL, usage | VK_IMAGE_USAGE_SAMPLED_BIT, 0 );
		//attachment->texture.createImageView ( aspectMask );

		attachment->description                = {};
		attachment->description.samples        = VK_SAMPLE_COUNT_1_BIT;
		attachment->description.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment->description.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachment->description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment->description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment->description.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment->description.finalLayout    = imageLayout;
		attachment->description.format         = format;

		attachments.push_back ( attachment );
		
		return *this;
	}
	
	Framebuffer&	create ()
	{
		VkFramebufferCreateInfo		createInfo = {};
		std::vector<VkImageView>	views;

		for ( size_t i = 0; i < attachments.size (); i++ )
		{
			auto&	desc = attachments [i]->description;
			
			if ( attachments [i]->texture.getImage ().hasDepth () )
			{
				renderpass.addAttachment   ( desc.format, desc.initialLayout, desc.finalLayout );	//VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
				renderpass.addDepthSubpass ( (uint32_t) i );
			}
			else
			{
				renderpass.addAttachment ( desc.format, desc.initialLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
				renderpass.addSubpass    ( (uint32_t) i );
			}
			
			views.push_back ( attachments [i]->texture.getImageView () );
		}
		
		renderpass.create ( *device );				// 2 dependencices ???
		
		createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass      = renderpass.getHandle ();
		createInfo.pAttachments    = views.data();
		createInfo.attachmentCount = (uint32_t) views.size ();
		createInfo.width           = width;
		createInfo.height          = height;
		createInfo.layers          = 1;
		
		if ( vkCreateFramebuffer ( device->getDevice (), &createInfo, nullptr, &framebuffer ) != VK_SUCCESS )
			fatal () << "Framebuffer: error creating framebuffer" << Log::endl;
		
		return *this;
	}
};
