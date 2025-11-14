

#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	<vulkan/vulkan.h>

#include	"mlx_config.h"
#include	"mlx_internal.h"

#include	"mlx___vulkan_internal.h"


/* not static */
void	*mlx___vulkan_img_error(mlx___vulkan_t *vk, mlx___vulkan_img_t *img,
				char *msg, VkResult err)
{
  int	i;
  
  if (msg)
    fprintf(stderr, "Mlx - VkImg: %x - %s\n", (int)err, msg);
  if (img)
    {
      // ensure out of any cmdbuff (?)
      mlx___vulkan_sync_internal(vk, NULL, img);
      vkDeviceWaitIdle(vk->vk_device); // pas subtil ...
      vkDestroySampler(vk->vk_device, img->texture_sampler, NULL);
      vkDestroyFramebuffer(vk->vk_device, img->img_fb, NULL);
      vkDestroyImageView(vk->vk_device, img->image_view, NULL);
      i = VK_NB_FRAMES;
      while (i--)
		{
		  if (img->staging_data[i])
			vkUnmapMemory(vk->vk_device, img->staging_device_memory[i]);
		  vkDestroyBuffer(vk->vk_device, img->staging_buffer[i], NULL);
		  vkFreeMemory(vk->vk_device, img->staging_device_memory[i], NULL);
		}
      vkFreeMemory(vk->vk_device, img->img_device_memory, NULL);
      vkDestroyImage(vk->vk_device, img->image, NULL);
      vk->img_ref[img->ref_idx].active = 0;
      vk->img_ref[img->ref_idx].item = (void *)0;
      free(img->data);
      free(img);
    }
  return (NULL);
}



void	mlx___vulkan_img_destroy(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_img_t            *vkimg;
  mlx___vulkan_t		*vk;

  vkimg = (mlx___vulkan_img_t *)(param->gpu_img);
  vk = (mlx___vulkan_t *)(param->gpu);
  if (vk && vkimg)
    {
      if (vkimg->draw_win_ref_idx >= 0 && vk->win_ref[vkimg->draw_win_ref_idx].active)
	mlx___vulkan_draw_internal(vk, (mlx___vulkan_win_t *)(vk->win_ref[vkimg->draw_win_ref_idx].item));
      mlx___vulkan_img_error(vk, vkimg, NULL, VK_SUCCESS);
    }
}


// not static
uint32_t	mlx___vulkan_get_mem_type(mlx___vulkan_t *vk, uint32_t filter,
					  VkMemoryPropertyFlags properties)
{
  int		i;

  i = 0;
  while (i < vk->device_mem_prop.memoryTypeCount)
    {
      if ((filter & (1 << i)) &&
	  (vk->device_mem_prop.memoryTypes[i].propertyFlags & properties) ==
	  properties)
	return i;
      i ++;
    }
  return (-1);
}


VkResult	mlx___vulkan_img_view(mlx___vulkan_t *vk,
				      mlx___vulkan_img_t *img)
{
  VkImageViewCreateInfo		img_view_crea_info;

  bzero(&img_view_crea_info, sizeof(img_view_crea_info));
  img_view_crea_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  img_view_crea_info.pNext = NULL;
  img_view_crea_info.flags = 0;
  img_view_crea_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  img_view_crea_info.format = img->format;
  img_view_crea_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  img_view_crea_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  img_view_crea_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  img_view_crea_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  img_view_crea_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  img_view_crea_info.subresourceRange.baseMipLevel = 0;
  img_view_crea_info.subresourceRange.levelCount = 1;
  img_view_crea_info.subresourceRange.baseArrayLayer = 0;
  img_view_crea_info.subresourceRange.layerCount = 1;
  img_view_crea_info.image = img->image;

  return (vkCreateImageView(vk->vk_device, &(img_view_crea_info), NULL, &(img->image_view)));
}



VkResult	mlx___vulkan_img_layout(VkCommandBuffer cmd_buffer,
					mlx___vulkan_img_t *vkimg,
					VkImageLayout to_layout)
{
  VkImageMemoryBarrier barrier;
  
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.pNext = NULL;
  barrier.oldLayout = vkimg->cur_layout;
  barrier.newLayout = to_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = vkimg->image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
		       NULL, 0, NULL, 1, &barrier);

  vkimg->cur_layout = to_layout;
  
  return (VK_SUCCESS);
}


VkResult	mlx___vulkan_img_sampler(mlx___vulkan_t *vk,
					 mlx___vulkan_img_t *vkimg)
{
  VkSamplerCreateInfo		sampler_create_info;

  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.pNext = NULL;
  sampler_create_info.flags = 0;
  sampler_create_info.magFilter = VK_FILTER_LINEAR;
  sampler_create_info.minFilter = VK_FILTER_LINEAR;
  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.anisotropyEnable = VK_FALSE;
  sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_create_info.unnormalizedCoordinates = VK_FALSE;
  sampler_create_info.compareEnable = VK_FALSE;
  sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_create_info.mipLodBias = 0.0f;
  sampler_create_info.minLod = 0.0f;
  sampler_create_info.maxLod = 0.0f;

  return (vkCreateSampler(vk->vk_device, &sampler_create_info, NULL,
			  &(vkimg->texture_sampler)));
}



void	*mlx___vulkan_img_create_internal(mlx___vulkan_t *vk,
					  uint width, uint height)
{
  mlx___vulkan_img_t		*vkimg;
  VkImageCreateInfo		image_create_info;
  VkMemoryRequirements		mem_requirements;
  VkMemoryAllocateInfo		alloc_info;
  VkResult			vkerr;
  int				i;
  
  if (width & (uint)0xFFFF8000 || height & (uint)0xFFFF8000)
    return (NULL);
  if ((vkimg = malloc(sizeof(*vkimg))) == NULL)
    return (NULL);
  bzero(vkimg, sizeof(*vkimg));
  vkimg->ref_idx = -1;
  if ((i = mlx___vulkan_ref_img_add(vk, vkimg)) == -1)
    return (mlx___vulkan_img_error(vk, vkimg, "ref_idx", VK_ERROR_UNKNOWN));
  vkimg->ref_idx = i;
  vkimg->draw_win_ref_idx = -1;
  vkimg->width = width;
  vkimg->height = height;
  vkimg->format = VK_FORMAT_B8G8R8A8_UNORM; // enforce this one. ok ?
  vkimg->size = vkimg->width * vkimg->height * 4;
  vkimg->cur_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.pNext = NULL;
  image_create_info.flags = 0;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.extent.width = vkimg->width;
  image_create_info.extent.height = vkimg->height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = 1;
  image_create_info.format = vkimg->format;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.initialLayout = vkimg->cur_layout;
  image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if ((vkerr = vkCreateImage(vk->vk_device, &image_create_info, NULL,
			     &(vkimg->image))) != VK_SUCCESS)
    return (mlx___vulkan_img_error(vk, vkimg, "VkCreateImage", vkerr));

  vkGetImageMemoryRequirements(vk->vk_device, vkimg->image, &mem_requirements);

  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext = NULL;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
    mlx___vulkan_get_mem_type(vk, mem_requirements.memoryTypeBits,
			      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (alloc_info.memoryTypeIndex == -1)
    return (mlx___vulkan_img_error(vk, vkimg, "No mem type",
				   VK_ERROR_UNKNOWN));
  if ((vkerr = vkAllocateMemory(vk->vk_device, &alloc_info, NULL,
				&(vkimg->img_device_memory))) != VK_SUCCESS)
    return (mlx___vulkan_img_error(vk, vkimg, "VkAlloc", vkerr));
    
  if ((vkerr = vkBindImageMemory(vk->vk_device, vkimg->image,
				 vkimg->img_device_memory, 0)) != VK_SUCCESS)
    return (mlx___vulkan_img_error(vk, vkimg, "VkBindImgMem", vkerr));

  /*
  can't map image memory on real GPU
  Use VK_NB_FRAMES staging buffers..
  */
  i = VK_NB_FRAMES;
  while (i--)
    {
      if ((vkerr = mlx___vulkan_create_buffer(vk, vkimg->size,
					      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
					      &(vkimg->staging_buffer[i]),
					      &(vkimg->staging_device_memory[i])))
	  != VK_SUCCESS)
	return (mlx___vulkan_img_error(vk, vkimg, "staging img buffer", vkerr));
      
      if ((vkerr = vkMapMemory(vk->vk_device, vkimg->staging_device_memory[i], 0,
			       vkimg->size, 0, &(vkimg->staging_data[i])))
	  != VK_SUCCESS)
	return (mlx___vulkan_img_error(vk, vkimg, "VkMapMem staging img", vkerr));
      vkimg->staging_status[i] = 0;
    }

  if ((vkimg->data = malloc(vkimg->size)) == NULL)
    return (mlx___vulkan_img_error(vk, vkimg, "malloc image data", vkerr));

  // create view
  if ((vkerr = mlx___vulkan_img_view(vk, vkimg)) != VK_SUCCESS)
    return (mlx___vulkan_img_error(vk, vkimg, "img view", vkerr));

  // create sampler
  if ((vkerr = mlx___vulkan_img_sampler(vk, vkimg)) != VK_SUCCESS)
    return (mlx___vulkan_img_error(vk, vkimg, "sampler", vkerr));

  vkimg->default_uniform.src_full[0] = vkimg->width;
  vkimg->default_uniform.src_full[1] = vkimg->height;
  vkimg->default_uniform.src_pos[0] = 0.0;
  vkimg->default_uniform.src_pos[1] = 0.0;
  vkimg->default_uniform.src_size[0] = vkimg->width;
  vkimg->default_uniform.src_size[1] = vkimg->height;
  vkimg->default_uniform.dst_size[0] = vkimg->width;
  vkimg->default_uniform.dst_size[1] = vkimg->height;
  vkimg->default_uniform.dst_full[0] = 0.0; // must change
  vkimg->default_uniform.dst_full[1] = 0.0; // must change
  vkimg->default_uniform.dst_pos[0] = 0.0;
  vkimg->default_uniform.dst_pos[1] = 0.0;
  vkimg->default_uniform.color[0] = 1.0;
  vkimg->default_uniform.color[1] = 1.0;
  vkimg->default_uniform.color[2] = 1.0;
  vkimg->default_uniform.color[3] = 1.0;
  
  return (vkimg);
}

void	*mlx___vulkan_img_create(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_img_t	*vkimg;
  
  vkimg = mlx___vulkan_img_create_internal((mlx___vulkan_t *)(param->gpu),
					   param->dst.width,
					   param->dst.height);
  param->data = vkimg->data;
  // format for mlx top level - 0 = B8G8R8A8; 1 = A8R8G8B8;
  param->format = 0;
  param->sizeline = 4 * vkimg->width;
  return (vkimg);
}


int	mlx___vulkan_img_put_internal(mlx___vulkan_t *vk,
				      mlx___vulkan_win_t *vkwin,
				      mlx___vulkan_img_t *vkimg,
				      mlx___vulkan_img_uniform_t *uniform)
{
  mlx___vulkan_draw_list_t	*dl;
  int				i;
  int				slot[3];
  int				myslot;
  unsigned int			minframe;

  slot[0] = slot[1] = slot[2] = -1;
  minframe = 0xFFFFFFFF;
  i = VK_NB_FRAMES;
  while (i--)
    {
      if (vkimg->staging_status[i] < 2)
	slot[vkimg->staging_status[i]] = i;
      if (vkimg->staging_status[i] == 2 && vkimg->staging_frame[i] < minframe)
	{
	  minframe = vkimg->staging_frame[i];
	  slot[2] = i;
	}
    }

  if (vkimg->draw_win_ref_idx >= 0 && vk->win_ref[vkimg->draw_win_ref_idx].active &&
      vkimg->draw_win_ref_idx != vkwin->ref_idx)
    {
      if (slot[1] >= 0)
	mlx___vulkan_draw_internal(vk, (mlx___vulkan_win_t *)vk->win_ref[vkimg->draw_win_ref_idx].item);
      mlx___vulkan_sync_internal(vk, (mlx___vulkan_win_t *)vk->win_ref[vkimg->draw_win_ref_idx].item, NULL); // wait vkimage used on other win
      i = VK_NB_FRAMES;
      while (i--)
	vkimg->staging_status[i] = 0; // clean all buffers
    }
  vkimg->draw_win_ref_idx = vkwin->ref_idx;

  if (slot[1] >= 0)
    myslot = slot[1];
  else
    {
      if (slot[0] >= 0)
	myslot = slot[0];
      else
	{
	  myslot = slot[2];
	  if (vkimg->staging_frame[myslot] > vkwin->cur_frame - VK_NB_FRAMES)
	    vkWaitForFences(vk->vk_device, 1, vkwin->frames[vkimg->staging_frame[myslot] % VK_NB_FRAMES].fence, VK_TRUE, UINT64_MAX);
	}
    }
  
  if (vkwin->draw_idx >= VK_NB_DRAW)
    mlx___vulkan_draw_internal(vk, vkwin);
  
  dl = vkwin->draw_list + vkwin->draw_idx;
  dl->img_ref_idx = vkimg->ref_idx;
  bcopy(uniform, &(dl->staging_uniform), sizeof(dl->staging_uniform));
  vkwin->draw_idx ++;
  dl->img_staging_idx = myslot;
  vkimg->staging_status[myslot] = 1;
  
  mlx___vulkan_img_mem_sync(vk, vkimg->data, vkimg->staging_data[myslot], vkimg->staging_device_memory[myslot], vkimg->size);
  return (0);
}


void mlx___vulkan_img_mem_sync(mlx___vulkan_t *vk, void *src, void *dst, VkDeviceMemory dst_gpu, VkDeviceSize size)
{
  VkMappedMemoryRange		mmr;

  memcpy(dst, src, size);
  mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mmr.pNext = NULL;
  mmr.memory = dst_gpu;
  mmr.offset = 0;
  mmr.size = size;
  vkFlushMappedMemoryRanges(vk->vk_device, 1, &mmr);
}

int	mlx___vulkan_img_put(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_t                *vk;
  mlx___vulkan_win_t            *vkwin;
  mlx___vulkan_img_t            *vkimg;
  mlx___vulkan_img_uniform_t	uniform;

  vk = (mlx___vulkan_t *)(param->gpu);
  vkwin = (mlx___vulkan_win_t *)(param->gpu_win);
  vkimg = (mlx___vulkan_img_t *)(param->gpu_img);
  if (!vk || !vkimg || !vkwin)
    return (1);

  bcopy(&(vkimg->default_uniform), &uniform, sizeof(uniform));
  uniform.dst_full[0] = vkwin->width;
  uniform.dst_full[1] = vkwin->height;
  uniform.dst_pos[0] = param->dst.x;
  uniform.dst_pos[1] = param->dst.y;

  if (param->dst.width != 0 && param->dst.height != 0)
    {
      uniform.dst_size[0] = param->dst.width;
      uniform.dst_size[1] = param->dst.height;
      uniform.src_pos[0] = param->src.x;
      uniform.src_pos[1] = param->src.y;
      uniform.src_size[0] = param->src.width;
      uniform.src_size[1] = param->src.height;
      uniform.color[0] = (float)(*(((unsigned char *)&param->color)+0)) / 255.0;
      uniform.color[1] = (float)(*(((unsigned char *)&param->color)+1)) / 255.0;
      uniform.color[2] = (float)(*(((unsigned char *)&param->color)+2)) / 255.0;
      uniform.color[3] = (float)(*(((unsigned char *)&param->color)+3)) / 255.0;
    }
  
  return (mlx___vulkan_img_put_internal(vk, vkwin, vkimg, &uniform));
}

