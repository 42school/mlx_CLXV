
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	<vulkan/vulkan.h>

#include	"mlx_config.h"
#include	"mlx_internal.h"


#if defined MLX_BACKEND && MLX_BACKEND == MLX_BACKEND_XCB
#include	<xcb/xcb.h>
#include	<xcb/xcb_keysyms.h>
#include	<vulkan/vulkan_xcb.h>
#include	"backend/mlx__xcb_internal.h"
#endif


#include	"mlx___vulkan_internal.h"


static void	*mlx___vulkan_window_error(mlx___vulkan_t *vk,
					   mlx___vulkan_win_t *vkwin,
					   char *msg, VkResult err)
{
  int	i;
  
  if (msg)
    fprintf(stderr, "Mlx - Vk Win: %x - %s\n", (int)err, msg);
  if (vkwin)
    {
      vkDeviceWaitIdle(vk->vk_device); // pas subtil ...

      if (vkwin->pixel_put_img)
		mlx___vulkan_img_error(vk, vkwin->pixel_put_img, NULL, VK_SUCCESS);
      if (vkwin->back_img)
		mlx___vulkan_img_error(vk, vkwin->back_img, NULL, VK_SUCCESS);

      i = 0;
      while (i < VK_NB_DRAW)
		{
		  if (vkwin->draw_list[i].uniform)
			vkUnmapMemory(vk->vk_device,
						  vkwin->draw_list[i].uniform_device_memory);
		  vkDestroyBuffer(vk->vk_device,
						  vkwin->draw_list[i].uniform_buffer, NULL);
		  vkFreeMemory(vk->vk_device,
					   vkwin->draw_list[i].uniform_device_memory, NULL);
		  i ++;
		}
      vkDestroyDescriptorPool(vk->vk_device, vkwin->descriptor_pool, NULL);

      i = 0;
      while (i < VK_NB_FRAMES)
		{
		  vkDestroyFence(vk->vk_device, vkwin->frames[i].fence[0], NULL);
		  i ++;
		}

      vkDestroyPipeline(vk->vk_device, vkwin->pipeline, NULL);

      if (vkwin->swch_images)
		{
		  i = 0;
		  while (i < vkwin->swch_images_nb)
			{
			  vkDestroyFramebuffer(vk->vk_device,
								   vkwin->swch_images[i].img_fb, NULL);
			  vkDestroyImageView(vk->vk_device,
								 vkwin->swch_images[i].image_view, NULL);
			  i ++;
			}
		  i = 3*(vkwin->swch_images_nb+1);
		  while (i--)
			vkDestroySemaphore(vk->vk_device, vkwin->swch_sema[i], NULL);
		  free(vkwin->swch_sema);
		  free(vkwin->swch_images);
		  free(vkwin->swch_fence);
		}

      vkDestroyRenderPass(vk->vk_device, vkwin->render_pass, NULL);
      vkDestroySwapchainKHR(vk->vk_device, vkwin->swap_chain, NULL);
      if (vkwin->surf_fmt)
		free(vkwin->surf_fmt);
      vkDestroySurfaceKHR(vk->instance, vkwin->surface, NULL);

      vk->win_ref[vkwin->ref_idx].active = 0;
      vk->win_ref[vkwin->ref_idx].item = (void *)0;
      free(vkwin);
    }
  return (NULL);
}


void	mlx___vulkan_window_destroy(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_t		*vk;
  mlx___vulkan_win_t            *vkwin;

  vk = (mlx___vulkan_t *)(param->gpu);
  vkwin = (mlx___vulkan_win_t *)(param->gpu_win);
  if (vk && vkwin)
    {
            // ensure out of any cmdbuff (?)
      mlx___vulkan_sync_internal(vk, vkwin, NULL);
      mlx___vulkan_window_error(vk, vkwin, NULL, VK_SUCCESS);
    }
}


VkResult	mlx___vulkan_create_buffer(mlx___vulkan_t *vk,
					   VkDeviceSize size,
					   VkBufferUsageFlags buff_usage,
					   VkMemoryPropertyFlags mem_properties,
					   VkBuffer *vk_buffer,
					   VkDeviceMemory *buffer_memory)
{
    VkBufferCreateInfo		buffer_create_info;
    VkMemoryRequirements	mem_requirements;
    VkMemoryAllocateInfo	alloc_info;
    VkResult			vkerr;

    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = NULL;
    buffer_create_info.flags = 0;
    buffer_create_info.size = size;
    buffer_create_info.usage = buff_usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if ((vkerr = vkCreateBuffer(vk->vk_device, &buffer_create_info, NULL,
				vk_buffer)) != VK_SUCCESS)
      return (vkerr);
    vkGetBufferMemoryRequirements(vk->vk_device, *vk_buffer,
				  &mem_requirements);

    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex =
      mlx___vulkan_get_mem_type(vk, mem_requirements.memoryTypeBits,
				mem_properties);

    if (alloc_info.memoryTypeIndex == -1)
      return (VK_ERROR_UNKNOWN);
      
    if ((vkerr = vkAllocateMemory(vk->vk_device, &alloc_info, NULL,
				  buffer_memory)) != VK_SUCCESS)
      return (vkerr);
    
    if ((vkerr = vkBindBufferMemory(vk->vk_device, *vk_buffer,
				    *buffer_memory, 0)) != VK_SUCCESS)
      return (vkerr);
    
    return (VK_SUCCESS);
}



static VkResult	mlx___vulkan_surface(mlx___vulkan_t *vk, mlx___vulkan_win_t *vkwin, mlx_gpu_hooks_param_t *param)
{
#if defined MLX_BACKEND && MLX_BACKEND == MLX_BACKEND_XCB
  VkXcbSurfaceCreateInfoKHR	surf_info;
  
  surf_info.pNext = NULL;
  surf_info.flags = 0;
  surf_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
  surf_info.connection = ((mlx__xcb_t *)(param->backend))->cnx;
  surf_info.window = ((mlx__xcb_win_t *)(param->backend_win))->win_id;
  return (vkCreateXcbSurfaceKHR(vk->instance, &(surf_info), NULL,
				&(vkwin->surface)));
#endif
  return (VK_ERROR_UNKNOWN);
}



static VkResult	mlx___vulkan_img_framebuffer(mlx___vulkan_t *vk,
					     mlx___vulkan_win_t *vkwin,
					     mlx___vulkan_img_t *img)
{
  VkImageView			attachments[1] = { img->image_view };
  VkFramebufferCreateInfo	img_fb_crea_info;

  bzero(&img_fb_crea_info, sizeof(img_fb_crea_info));
  img_fb_crea_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  img_fb_crea_info.renderPass = vkwin->render_pass;
  img_fb_crea_info.attachmentCount = 1;
  img_fb_crea_info.pAttachments = attachments;
  img_fb_crea_info.width = img->width;
  img_fb_crea_info.height = img->height;
  img_fb_crea_info.layers = 1;

  return (vkCreateFramebuffer(vk->vk_device, &img_fb_crea_info, NULL,
			      &(img->img_fb)));
}



static int	mlx___vulkan_sw_images(mlx___vulkan_t *vk,
				       mlx___vulkan_win_t *vkwin)
{
  VkImage	*imgs;
  int		i;
  VkSemaphoreCreateInfo		submit_sem_crea_info;
  
  if (vkGetSwapchainImagesKHR(vk->vk_device, vkwin->swap_chain,
			      &(vkwin->swch_images_nb), NULL) != VK_SUCCESS)
    return (1);
  if ((vkwin->swch_images = calloc(vkwin->swch_images_nb,
				   sizeof(*(vkwin->swch_images)))) == NULL)
    return (1);
  if ((imgs = calloc(vkwin->swch_images_nb, sizeof(*imgs))) == NULL)
    return (1);
  if (vkGetSwapchainImagesKHR(vk->vk_device, vkwin->swap_chain,
			      &(vkwin->swch_images_nb), imgs) != VK_SUCCESS)
    return (1);

  if ((vkwin->swch_fence = calloc(vkwin->swch_images_nb,
				  sizeof(*(vkwin->swch_fence)))) == NULL)
    return (1);
  if ((vkwin->swch_sema = calloc(3*(vkwin->swch_images_nb+1),
				 sizeof(*(vkwin->swch_sema)))) == NULL)
    return (1);
  
  bzero(&submit_sem_crea_info, sizeof(submit_sem_crea_info));
  submit_sem_crea_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  submit_sem_crea_info.pNext = NULL;
  submit_sem_crea_info.flags = 0;

  i = 3*(vkwin->swch_images_nb+1);
  while (i--)
    if (vkCreateSemaphore(vk->vk_device, &submit_sem_crea_info,
			  NULL, vkwin->swch_sema+i)
	!= VK_SUCCESS)
      return (1);
  vkwin->swch_sema_idx = -1;
      
  i = 0;
  while (i < vkwin->swch_images_nb)
    {
      vkwin->swch_images[i].image = imgs[i];
      vkwin->swch_images[i].format = vkwin->format;
      vkwin->swch_images[i].width = vkwin->width;
      vkwin->swch_images[i].height = vkwin->height;
      vkwin->swch_images[i].cur_layout = VK_IMAGE_LAYOUT_UNDEFINED;
      if (mlx___vulkan_img_view(vk, vkwin->swch_images+i) != VK_SUCCESS)
	return (1);
      if (mlx___vulkan_img_framebuffer(vk, vkwin, vkwin->swch_images+i) != VK_SUCCESS)
	return (1);
      vkwin->swch_fence[i] = VK_NULL_HANDLE;
      i ++;
    }

  free(imgs);
  return (0);
}



static int	mlx___vulkan_swap_chain(mlx___vulkan_t *vk,
					mlx___vulkan_win_t *vkwin,
					mlx_gpu_hooks_param_t *param)
{
  int				i;
  VkSwapchainCreateInfoKHR	swch_crea_info;

  /* first colorspace and format */
  vkwin->sfmt_nb = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk->devices[vk->dev], vkwin->surface,
				       &(vkwin->sfmt_nb), NULL);
  if (vkwin->sfmt_nb < 1 ||
      (vkwin->surf_fmt = malloc(vkwin->sfmt_nb * sizeof(*(vkwin->surf_fmt))))
      == NULL)
    return (VK_ERROR_UNKNOWN);
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk->devices[vk->dev], vkwin->surface,
				       &(vkwin->sfmt_nb), vkwin->surf_fmt);
  vkwin->format = -1;
  i = 0;
  while (i < vkwin->sfmt_nb)
    {
      // printf("surface formats - id: %d - fmt %d colspc %d\n", i, (vkwin->surf_fmt+i)->format, (vkwin->surf_fmt+i)->colorSpace);
      if ((vkwin->surf_fmt+i)->format == VK_FORMAT_UNDEFINED ||
	  (vkwin->surf_fmt+i)->format == VK_FORMAT_B8G8R8A8_UNORM )
	{
	  vkwin->format = VK_FORMAT_B8G8R8A8_UNORM;
	  vkwin->color_space = (vkwin->surf_fmt+i)->colorSpace;
	}
      if (vkwin->format == -1 && ((vkwin->surf_fmt+i)->format
				  == VK_FORMAT_B8G8R8A8_SRGB))
	{
	  vkwin->format = VK_FORMAT_B8G8R8A8_SRGB;
	  vkwin->color_space = (vkwin->surf_fmt+i)->colorSpace;
	}
      i ++;
    }
  // printf("=> selected fmt %d - colspc %d\n", vkwin->format, vkwin->color_space);

  /* then capabilities */
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk->devices[vk->dev],
						vkwin->surface,
						&(vkwin->surf_cap))
      != VK_SUCCESS)
    return (VK_ERROR_UNKNOWN);

  // printf("surf cap min-max : %d %d\n", vkwin->surf_cap.minImageCount, vkwin->surf_cap.maxImageCount);

  vkwin->width = (vkwin->surf_cap.currentExtent.width==-1)?param->dst.width:vkwin->surf_cap.currentExtent.width;
  vkwin->height = (vkwin->surf_cap.currentExtent.height==-1)?param->dst.height:vkwin->surf_cap.currentExtent.height;

  /* finally fill out crea info and create swapchain */
  swch_crea_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swch_crea_info.pNext = NULL;
  swch_crea_info.flags = 0;
  swch_crea_info.surface = vkwin->surface;
  swch_crea_info.minImageCount = MAX(vkwin->surf_cap.minImageCount, MIN(2, vkwin->surf_cap.maxImageCount==0?9999:vkwin->surf_cap.maxImageCount));
  swch_crea_info.imageFormat = vkwin->format;
  swch_crea_info.imageColorSpace = vkwin->color_space;
  // should instead check for MAXINT uint32_t, and if using param->w/h, control it's within
  // surf_cap.minImageExtent and surf_cap.maxImageExtent
  swch_crea_info.imageExtent.width = vkwin->width;
  swch_crea_info.imageExtent.height = vkwin->height;
  swch_crea_info.imageArrayLayers = 1;
  swch_crea_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  swch_crea_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swch_crea_info.queueFamilyIndexCount = 0;
  swch_crea_info.pQueueFamilyIndices = NULL;
  if (vkwin->surf_cap.supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    swch_crea_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  else
    swch_crea_info.preTransform = vkwin->surf_cap.currentTransform;
  swch_crea_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swch_crea_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swch_crea_info.clipped = VK_TRUE;
  swch_crea_info.oldSwapchain = VK_NULL_HANDLE;
  
  return (vkCreateSwapchainKHR(vk->vk_device, &swch_crea_info, NULL, &(vkwin->swap_chain)));
}





static VkResult	mlx___vulkan_pipeline(mlx___vulkan_t *vk,
				      mlx___vulkan_win_t *vkwin)
{
  struct					mlx___vulkan_pipeline_s
  {
    VkGraphicsPipelineCreateInfo		pipeline_crea_info;
    VkPipelineShaderStageCreateInfo		shader_stages[2];
    VkPipelineVertexInputStateCreateInfo	pl_vert_input_crea_info;
    VkPipelineInputAssemblyStateCreateInfo	pl_input_asm_crea_info;
    VkViewport					viewport;
    VkRect2D					scissor;
    VkPipelineViewportStateCreateInfo		pl_vpst_crea_info;
    VkPipelineRasterizationStateCreateInfo	rasterizer;
    VkPipelineMultisampleStateCreateInfo	multisampling;
    VkPipelineDepthStencilStateCreateInfo	pl_depth_crea_info;
    VkPipelineColorBlendAttachmentState		color_blend_attch;
    VkPipelineColorBlendStateCreateInfo		color_blend;
  } pl_crea_info;

  bzero(&(pl_crea_info), sizeof(pl_crea_info));
  
  pl_crea_info.shader_stages[0].sType =
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pl_crea_info.shader_stages[0].pNext = NULL;
  pl_crea_info.shader_stages[0].flags = 0;
  pl_crea_info.shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  pl_crea_info.shader_stages[0].module = vk->shader_vert;
  pl_crea_info.shader_stages[0].pName = "main";

  pl_crea_info.shader_stages[1].sType =
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pl_crea_info.shader_stages[1].pNext = NULL;
  pl_crea_info.shader_stages[1].flags = 0;
  pl_crea_info.shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  pl_crea_info.shader_stages[1].module = vk->shader_frag;
  pl_crea_info.shader_stages[1].pName = "main";

  pl_crea_info.pl_vert_input_crea_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  pl_crea_info.pl_vert_input_crea_info.pNext = NULL;
  pl_crea_info.pl_vert_input_crea_info.flags = 0;
  pl_crea_info.pl_vert_input_crea_info.vertexBindingDescriptionCount = 0;
  pl_crea_info.pl_vert_input_crea_info.pVertexBindingDescriptions = NULL;
  pl_crea_info.pl_vert_input_crea_info.vertexAttributeDescriptionCount = 0;
  pl_crea_info.pl_vert_input_crea_info.pVertexAttributeDescriptions = NULL;

  pl_crea_info.pl_input_asm_crea_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  pl_crea_info.pl_input_asm_crea_info.pNext = NULL;
  pl_crea_info.pl_input_asm_crea_info.flags = 0;
  pl_crea_info.pl_input_asm_crea_info.topology =
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  pl_crea_info.pl_input_asm_crea_info.primitiveRestartEnable = VK_FALSE;

  pl_crea_info.viewport.x = 0.0f;
  pl_crea_info.viewport.y = 0.0f;
  pl_crea_info.viewport.width = vkwin->width;
  pl_crea_info.viewport.height = vkwin->height;
  pl_crea_info.viewport.minDepth = 0.0f;
  pl_crea_info.viewport.maxDepth = 1.0f;

  pl_crea_info.scissor.offset.x = 0;
  pl_crea_info.scissor.offset.y = 0;
  pl_crea_info.scissor.extent.width = vkwin->width;
  pl_crea_info.scissor.extent.height = vkwin->height;

  pl_crea_info.pl_vpst_crea_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  pl_crea_info.pl_vpst_crea_info.pNext = NULL;
  pl_crea_info.pl_vpst_crea_info.flags = 0;
  pl_crea_info.pl_vpst_crea_info.viewportCount = 1;
  pl_crea_info.pl_vpst_crea_info.pViewports = &(pl_crea_info.viewport);
  pl_crea_info.pl_vpst_crea_info.scissorCount = 1;
  pl_crea_info.pl_vpst_crea_info.pScissors = &(pl_crea_info.scissor);

  pl_crea_info.rasterizer.sType =
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  pl_crea_info.rasterizer.pNext = NULL;
  pl_crea_info.rasterizer.flags = 0;
  pl_crea_info.rasterizer.depthClampEnable = VK_FALSE;
  pl_crea_info.rasterizer.rasterizerDiscardEnable = VK_FALSE;
  pl_crea_info.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  pl_crea_info.rasterizer.lineWidth = 1.0f;
  pl_crea_info.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  pl_crea_info.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  pl_crea_info.rasterizer.depthBiasEnable = VK_FALSE;
  pl_crea_info.rasterizer.depthBiasConstantFactor = 0.0f;
  pl_crea_info.rasterizer.depthBiasClamp = 0.0f;
  pl_crea_info.rasterizer.depthBiasSlopeFactor = 0.0f;

  pl_crea_info.multisampling.sType =
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  pl_crea_info.multisampling.pNext = NULL;
  pl_crea_info.multisampling.flags = 0;
  pl_crea_info.multisampling.sampleShadingEnable = VK_FALSE;
  pl_crea_info.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  pl_crea_info.multisampling.minSampleShading = 1.0f;
  pl_crea_info.multisampling.pSampleMask = NULL;
  pl_crea_info.multisampling.alphaToCoverageEnable = VK_FALSE;
  pl_crea_info.multisampling.alphaToOneEnable = VK_FALSE;

  pl_crea_info.pl_depth_crea_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  pl_crea_info.pl_depth_crea_info.pNext = NULL;
  pl_crea_info.pl_depth_crea_info.flags =
    VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
  pl_crea_info.pl_depth_crea_info.depthTestEnable = VK_FALSE;
  pl_crea_info.pl_depth_crea_info.depthWriteEnable = VK_FALSE;
  pl_crea_info.pl_depth_crea_info.depthCompareOp = VK_COMPARE_OP_NEVER;
  pl_crea_info.pl_depth_crea_info.depthBoundsTestEnable = VK_FALSE;
  pl_crea_info.pl_depth_crea_info.stencilTestEnable = VK_FALSE;
  pl_crea_info.pl_depth_crea_info.front.failOp = VK_STENCIL_OP_KEEP;
  pl_crea_info.pl_depth_crea_info.front.passOp = VK_STENCIL_OP_KEEP;
  pl_crea_info.pl_depth_crea_info.front.depthFailOp = VK_STENCIL_OP_KEEP;
  pl_crea_info.pl_depth_crea_info.front.compareOp = VK_COMPARE_OP_NEVER;
  pl_crea_info.pl_depth_crea_info.front.compareMask = 0;
  pl_crea_info.pl_depth_crea_info.front.writeMask = 0;
  pl_crea_info.pl_depth_crea_info.front.reference = 0;
  pl_crea_info.pl_depth_crea_info.back.failOp = VK_STENCIL_OP_KEEP;
  pl_crea_info.pl_depth_crea_info.back.passOp = VK_STENCIL_OP_KEEP;
  pl_crea_info.pl_depth_crea_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
  pl_crea_info.pl_depth_crea_info.back.compareOp = VK_COMPARE_OP_NEVER;
  pl_crea_info.pl_depth_crea_info.back.compareMask = 0;
  pl_crea_info.pl_depth_crea_info.back.writeMask = 0;
  pl_crea_info.pl_depth_crea_info.back.reference = 0;
  pl_crea_info.pl_depth_crea_info.minDepthBounds = 0.0;
  pl_crea_info.pl_depth_crea_info.maxDepthBounds = 1.0;
  
  pl_crea_info.color_blend_attch.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  pl_crea_info.color_blend_attch.blendEnable = VK_TRUE;
  pl_crea_info.color_blend_attch.srcColorBlendFactor =
    VK_BLEND_FACTOR_SRC_ALPHA;
  pl_crea_info.color_blend_attch.dstColorBlendFactor =
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  pl_crea_info.color_blend_attch.colorBlendOp = VK_BLEND_OP_ADD;
  pl_crea_info.color_blend_attch.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  pl_crea_info.color_blend_attch.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  pl_crea_info.color_blend_attch.alphaBlendOp = VK_BLEND_OP_ADD;

  pl_crea_info.color_blend.sType =
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  pl_crea_info.color_blend.pNext = NULL;
  pl_crea_info.color_blend.flags = 0;
  pl_crea_info.color_blend.logicOpEnable = VK_FALSE;
  pl_crea_info.color_blend.logicOp = VK_LOGIC_OP_COPY;
  pl_crea_info.color_blend.attachmentCount = 1;
  pl_crea_info.color_blend.pAttachments = &(pl_crea_info.color_blend_attch);
  pl_crea_info.color_blend.blendConstants[0] = 0.0f;
  pl_crea_info.color_blend.blendConstants[1] = 0.0f;
  pl_crea_info.color_blend.blendConstants[2] = 0.0f;
  pl_crea_info.color_blend.blendConstants[3] = 0.0f;

  // finally ...
  
  pl_crea_info.pipeline_crea_info.sType =
    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pl_crea_info.pipeline_crea_info.pNext = NULL;
  pl_crea_info.pipeline_crea_info.flags = 0;
  pl_crea_info.pipeline_crea_info.stageCount = 2;
  pl_crea_info.pipeline_crea_info.pStages = pl_crea_info.shader_stages;
  pl_crea_info.pipeline_crea_info.pVertexInputState =
    &(pl_crea_info.pl_vert_input_crea_info);
  pl_crea_info.pipeline_crea_info.pInputAssemblyState =
    &(pl_crea_info.pl_input_asm_crea_info);
  pl_crea_info.pipeline_crea_info.pTessellationState = NULL;
  pl_crea_info.pipeline_crea_info.pViewportState =
    &(pl_crea_info.pl_vpst_crea_info);
  pl_crea_info.pipeline_crea_info.pRasterizationState =
    &(pl_crea_info.rasterizer);
  pl_crea_info.pipeline_crea_info.pMultisampleState =
    &(pl_crea_info.multisampling);
  pl_crea_info.pipeline_crea_info.pDepthStencilState =
    &(pl_crea_info.pl_depth_crea_info);
  pl_crea_info.pipeline_crea_info.pColorBlendState =
    &(pl_crea_info.color_blend);
  pl_crea_info.pipeline_crea_info.pDynamicState = NULL;
  pl_crea_info.pipeline_crea_info.layout = vk->pipeline_layout;
  pl_crea_info.pipeline_crea_info.renderPass = vkwin->render_pass;
  pl_crea_info.pipeline_crea_info.subpass = 0;
  pl_crea_info.pipeline_crea_info.basePipelineHandle = NULL;
  pl_crea_info.pipeline_crea_info.basePipelineIndex = -1;
  
  return (vkCreateGraphicsPipelines(vk->vk_device, VK_NULL_HANDLE, 1,
				    &(pl_crea_info.pipeline_crea_info),
				    NULL, &(vkwin->pipeline)));
}


static VkResult	mlx___vulkan_renderpass(mlx___vulkan_t *vk,
					mlx___vulkan_win_t *vkwin)
{
  VkRenderPassCreateInfo	rp_crea_info;
  VkAttachmentReference		col_attch_ref;
  VkSubpassDescription		subpass;
  VkAttachmentDescription	color_attch;
  VkSubpassDependency		subpass_dep;
  
  color_attch.flags = 0;
  color_attch.format = vkwin->format;
  color_attch.samples = VK_SAMPLE_COUNT_1_BIT;
  // no clear, will start with copy image - was VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attch.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  color_attch.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attch.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attch.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attch.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  color_attch.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; => render in back_img, no present but copied to sw image
    
  col_attch_ref.attachment = 0;
  col_attch_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = NULL;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &col_attch_ref;
  subpass.pResolveAttachments = NULL;
  subpass.pDepthStencilAttachment = NULL;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = NULL;

  subpass_dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dep.dstSubpass = 0;
  subpass_dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dep.srcAccessMask = 0;
  subpass_dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpass_dep.dependencyFlags = 0;
  
  rp_crea_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rp_crea_info.pNext = NULL;
  rp_crea_info.flags = 0;
  rp_crea_info.attachmentCount = 1;
  rp_crea_info.pAttachments = &color_attch;
  rp_crea_info.subpassCount = 1;
  rp_crea_info.pSubpasses = &subpass;
  rp_crea_info.dependencyCount = 1;
  rp_crea_info.pDependencies = &subpass_dep;

  return (vkCreateRenderPass(vk->vk_device, &rp_crea_info, NULL,
			     &(vkwin->render_pass)));
}


static VkResult	mlx___vulkan_command_buffer_alloc(mlx___vulkan_t *vk,
						  mlx___vulkan_win_t *vkwin)
{
  VkCommandBufferAllocateInfo cmdbuff_alloc_info;
  
  cmdbuff_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdbuff_alloc_info.pNext = NULL;
  cmdbuff_alloc_info.commandPool = vk->command_pool;
  cmdbuff_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  // one for render passes, one for backup frame
  cmdbuff_alloc_info.commandBufferCount = 2;

  return (vkAllocateCommandBuffers(vk->vk_device, &cmdbuff_alloc_info,
				   vkwin->frames[vkwin->cur_frame].cmd_buff));
}


static void *mlx___vulkan_window_frame_init(mlx___vulkan_t *vk, mlx___vulkan_win_t *vkwin)
{
  VkFenceCreateInfo		fence_crea_info;
  VkResult			vkerr;

  // 1 fences for each submit per frame
  bzero(&fence_crea_info, sizeof(fence_crea_info));
  fence_crea_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_crea_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  fence_crea_info.pNext = NULL;
  if ((vkerr = vkCreateFence(vk->vk_device, &fence_crea_info,
			     NULL, (vkwin->frames[vkwin->cur_frame].fence)))
      != VK_SUCCESS)
    return (mlx___vulkan_window_error(vk, vkwin, "fence-1", vkerr));

  if ((vkerr = mlx___vulkan_command_buffer_alloc(vk, vkwin)) != VK_SUCCESS)
    return (mlx___vulkan_window_error(vk, vkwin, "command buffers", vkerr));

  return ((void *)(1));
}


VkResult	mlx___vulkan_window_descriptor(mlx___vulkan_t *vk,
					       mlx___vulkan_win_t *vkwin)
{
  VkDescriptorPoolSize		pool_size[2];
  VkDescriptorPoolCreateInfo	pool_crea_info;
  VkDescriptorSetAllocateInfo	set_crea_info;
  VkDescriptorSetLayout		layouts[VK_NB_DRAW];
  int				i;
  VkResult			vkerr;

  i = 0;
  while (i < VK_NB_DRAW)
    layouts[i++] = vk->descriptor_set_layout;

  // create descriptor pool per window
  pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size[0].descriptorCount = VK_NB_DRAW * VK_NB_FRAMES;
  pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size[1].descriptorCount = VK_NB_DRAW * VK_NB_FRAMES;

  pool_crea_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_crea_info.pNext = NULL;
  pool_crea_info.flags = 0;
  pool_crea_info.poolSizeCount = 2;
  pool_crea_info.pPoolSizes = pool_size;
  pool_crea_info.maxSets = VK_NB_DRAW * VK_NB_FRAMES;
  if ((vkerr = vkCreateDescriptorPool(vk->vk_device, &pool_crea_info, NULL,
				      &(vkwin->descriptor_pool)))
      != VK_SUCCESS)
    return (vkerr);
  
  // allocate draw_list's descriptor sets from pool, for each frame.
  i = VK_NB_FRAMES;
  while (i--)
    {
      set_crea_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      set_crea_info.pNext = NULL;
      set_crea_info.descriptorPool = vkwin->descriptor_pool;
      set_crea_info.descriptorSetCount = VK_NB_DRAW;
      set_crea_info.pSetLayouts = layouts;
      if ((vkerr = vkAllocateDescriptorSets(vk->vk_device, &set_crea_info,
					    vkwin->frames[i].descriptor_set)) != VK_SUCCESS)
	return (vkerr);
    }
  return (VK_SUCCESS);
}



void	*mlx___vulkan_window(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_t	*vk;
  mlx___vulkan_win_t	*vkwin;
  int			i;
  VkResult		vkerr;

  vk = (mlx___vulkan_t *)(param->gpu);
  
  if ((vkwin = malloc(sizeof(*vkwin))) == NULL)
    return (NULL);
  bzero(vkwin, sizeof(*vkwin));
  vkwin->ref_idx = -1;
  if ((i = mlx___vulkan_ref_win_add(vk, vkwin)) == -1)
    return (mlx___vulkan_window_error(vk, vkwin, "ref_idx", VK_ERROR_UNKNOWN));
  vkwin->ref_idx = i;
  vkwin->vk = vk;

  if ((vkerr = mlx___vulkan_surface(vk, vkwin, param)) != VK_SUCCESS)
    return (mlx___vulkan_window_error(vk, vkwin, "surface", vkerr));
  
  if ((vkerr = mlx___vulkan_swap_chain(vk, vkwin, param)) != VK_SUCCESS)
    return (mlx___vulkan_window_error(vk, vkwin, "swapchain", vkerr));
    
  if ((vkerr = mlx___vulkan_renderpass(vk, vkwin)) != VK_SUCCESS)
    return (mlx___vulkan_window_error(vk, vkwin, "render pass", vkerr));

  if ((vkerr = mlx___vulkan_sw_images(vk, vkwin)) != VK_SUCCESS)
    return (mlx___vulkan_window_error(vk, vkwin, "sw img", vkerr));
  
  if ((vkerr = mlx___vulkan_pipeline(vk, vkwin)) != VK_SUCCESS)
    return (mlx___vulkan_window_error(vk, vkwin, "graphic pipeline", vkerr));
  
  vkwin->cur_frame = 0;
  while (vkwin->cur_frame < VK_NB_FRAMES)
    {
      if (mlx___vulkan_window_frame_init(vk, vkwin) == NULL)
	return (NULL);
      vkwin->cur_frame ++;
    }
  vkwin->cur_frame = 0;

  /* descriptors and draw list */
  if ((vkerr = mlx___vulkan_window_descriptor(vk, vkwin)) != VK_SUCCESS)
    return (mlx___vulkan_window_error(vk, vkwin, "desctiptors", vkerr));
  i = 0;
  while (i < VK_NB_DRAW)
    {
      vkwin->draw_list[i].img_ref_idx = -1;
      // create associated uniform buffer
      if ((vkerr =
	   mlx___vulkan_create_buffer(vk,
				      sizeof(*(vkwin->draw_list[i].uniform)),
				      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				      &(vkwin->draw_list[i].uniform_buffer),
				      &(vkwin->draw_list[i].uniform_device_memory)))
	  != VK_SUCCESS)
	return (mlx___vulkan_window_error(vk, vkwin, "uniform buffer", vkerr));
      if ((vkerr = vkMapMemory(vk->vk_device,
			       vkwin->draw_list[i].uniform_device_memory, 0,
			       sizeof(*(vkwin->draw_list[i].uniform)), 0,
			       (void **)(&(vkwin->draw_list[i].uniform))))
	  != VK_SUCCESS)
	return (mlx___vulkan_window_error(vk, vkwin, "VkMapMem uniform", vkerr));
      i ++;
    }
  
  if ((vkwin->back_img =
       mlx___vulkan_img_create_internal(vk, vkwin->width, vkwin->height))
      == NULL)
    return (mlx___vulkan_window_error(vk, vkwin, "back_image_create", VK_ERROR_UNKNOWN));
  vkwin->back_img->default_uniform.dst_full[0] = vkwin->width;
  vkwin->back_img->default_uniform.dst_full[1] = vkwin->height;
  // create FB from backimage for offscreen rendering
  if (mlx___vulkan_img_framebuffer(vk, vkwin, vkwin->back_img) != VK_SUCCESS)
    return (mlx___vulkan_window_error(vk, vkwin, "back_image_framebuffer", VK_ERROR_UNKNOWN));
  
  
  if ((vkwin->pixel_put_img =
       mlx___vulkan_img_create_internal(vk, vkwin->width, vkwin->height))
      == NULL)
    return (mlx___vulkan_window_error(vk, vkwin, "pixel_image_create", VK_ERROR_UNKNOWN));
  vkwin->pixel_put_img->default_uniform.dst_full[0] = vkwin->width;
  vkwin->pixel_put_img->default_uniform.dst_full[1] = vkwin->height;
  vkwin->pixel_put_img->draw_win_ref_idx = vkwin->ref_idx;
  vkwin->pixel_put_img_sync = -1;
  
  // set copy regions if used with VkCopyImage
  vkwin->copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vkwin->copy_region.srcSubresource.mipLevel = 0;
  vkwin->copy_region.srcSubresource.baseArrayLayer = 0;
  vkwin->copy_region.srcSubresource.layerCount = 1;
  vkwin->copy_region.srcOffset = (VkOffset3D){0, 0, 0};
  vkwin->copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vkwin->copy_region.dstSubresource.mipLevel = 0;
  vkwin->copy_region.dstSubresource.baseArrayLayer = 0;
  vkwin->copy_region.dstSubresource.layerCount = 1;
  vkwin->copy_region.dstOffset = (VkOffset3D){0, 0, 0};
  vkwin->copy_region.extent = (VkExtent3D){vkwin->width, vkwin->height, 1};

  vkwin->clear_it = 1;

  return (vkwin);
}


int	mlx___vulkan_window_clear(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_t	*vk;
  mlx___vulkan_win_t	*vkwin;

  vk = (mlx___vulkan_t *)(param->gpu);
  vkwin = (mlx___vulkan_win_t *)(param->gpu_win);

  mlx___vulkan_sync_internal(vk, vkwin, NULL);
  vkwin->clear_it = 1;
  return (0);
}


int	mlx___vulkan_window_pixel(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_t	*vk;
  mlx___vulkan_win_t	*vkwin;
  unsigned int		*pixel;
  int			i;

  vk = (mlx___vulkan_t *)(param->gpu);
  vkwin = (mlx___vulkan_win_t *)(param->gpu_win);
  if (param->dst.x < 0 || param->dst.x >= vkwin->width ||
	  param->dst.y < 0 || param->dst.y >= vkwin->height)
	return (0);
  pixel = (unsigned int *)(vkwin->pixel_put_img->data + param->dst.x*4 +
			   param->dst.y*4*vkwin->pixel_put_img->width);
  if (vkwin->draw_idx > 0
      && vkwin->draw_list[vkwin->draw_idx-1].img_ref_idx == vkwin->pixel_put_img->ref_idx)
    *pixel = param->color;
  else
    {
      i = VK_NB_FRAMES;
      while (i--)
	if (vkwin->pixel_put_img->staging_status[i] == 1)
	  mlx___vulkan_draw_internal(vk, vkwin);
      // mlx___vulkan_sync_internal(vk, vkwin, vkwin->pixel_put_img); done by img_put_internal if needed
      bzero(vkwin->pixel_put_img->data,
	    vkwin->pixel_put_img->width*vkwin->pixel_put_img->height*4);
      *pixel = param->color;
      mlx___vulkan_img_put_internal(vk, vkwin, vkwin->pixel_put_img,
				    &(vkwin->pixel_put_img->default_uniform));
      vkwin->pixel_put_img_sync = vkwin->draw_list[vkwin->draw_idx-1].img_staging_idx;
    }
  return (0);
}
