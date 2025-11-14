

#include	<stdlib.h>
#include	<stdio.h>
#include	<strings.h>

#include	"mlx_internal.h"

#include	<vulkan/vulkan.h>

#include	"mlx___vulkan_internal.h"



static VkResult	mlx___vulkan_desc_write(mlx___vulkan_t *vk,
					mlx___vulkan_draw_list_t *dl)
{
  VkDescriptorBufferInfo	desc_buffer_info;
  VkDescriptorImageInfo		desc_image_info;
  VkWriteDescriptorSet		desc_write[2];
  
  // update (bind) texture sampler & view (and unchanged uniform buffer)
  
  desc_buffer_info.buffer = dl->uniform_buffer;
  desc_buffer_info.offset = 0;
  desc_buffer_info.range = sizeof(*(dl->uniform));

  desc_image_info.sampler = ((mlx___vulkan_img_t *)(vk->img_ref[dl->img_ref_idx].item))->texture_sampler;
  desc_image_info.imageView = ((mlx___vulkan_img_t *)(vk->img_ref[dl->img_ref_idx].item))->image_view;
  desc_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  desc_write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  desc_write[0].pNext = NULL;
  desc_write[0].dstSet = *(dl->descriptor_set);
  desc_write[0].dstBinding = 0;
  desc_write[0].dstArrayElement = 0;
  desc_write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  desc_write[0].descriptorCount = 1;
  desc_write[0].pBufferInfo = &desc_buffer_info;
  desc_write[0].pImageInfo = NULL;
  desc_write[0].pTexelBufferView = NULL;

  desc_write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  desc_write[1].pNext = NULL;
  desc_write[1].dstSet = *(dl->descriptor_set);
  desc_write[1].dstBinding = 1;
  desc_write[1].dstArrayElement = 0;
  desc_write[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  desc_write[1].descriptorCount = 1;
  desc_write[1].pBufferInfo = NULL;
  desc_write[1].pImageInfo = &desc_image_info;
  desc_write[1].pTexelBufferView = NULL;
  
  vkUpdateDescriptorSets(vk->vk_device, 2, desc_write, 0, NULL);
  return (VK_SUCCESS);
}



static int	mlx___vulkan_command_buffer_set_renderpass(mlx___vulkan_t *vk, mlx___vulkan_win_t *vkwin, int swch_index)
{
  mlx___vulkan_win_frame_t	*frame;
  VkCommandBufferBeginInfo	cmdbuff_begin_info;
  VkRenderPassBeginInfo		render_pass_info;
  VkClearValue			clear_value;
  VkClearColorValue		clear_color;
  VkImageSubresourceRange	clear_range;
  int				i;
  VkBufferImageCopy		region;
  mlx___vulkan_img_t		*img;

  frame = vkwin->frames + (vkwin->cur_frame % VK_NB_FRAMES);
  bzero(&region, sizeof(region));
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount = 1;
  region.imageExtent.depth = 1;
  
  cmdbuff_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdbuff_begin_info.pNext = NULL;
  cmdbuff_begin_info.flags = 0;
  cmdbuff_begin_info.pInheritanceInfo = NULL;

  if (vkBeginCommandBuffer(frame->cmd_buff[0], &cmdbuff_begin_info)
      != VK_SUCCESS)
    return (1);

  i = 0;
  while (i < vkwin->draw_idx)
    {
      if (vkwin->draw_list[i].img_ref_idx >= 0 &&
	  vk->img_ref[vkwin->draw_list[i].img_ref_idx].active)
	{
	  img = (mlx___vulkan_img_t *)vk->img_ref[vkwin->draw_list[i].img_ref_idx].item;
	  mlx___vulkan_img_layout(frame->cmd_buff[0], img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	  region.imageExtent.width = img->width;
	  region.imageExtent.height = img->height;
	  vkCmdCopyBufferToImage(frame->cmd_buff[0],
				 img->staging_buffer[vkwin->draw_list[i].img_staging_idx],
				 img->image,
				 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				 1, &region);
	  mlx___vulkan_img_layout(frame->cmd_buff[0], img,
			      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
      i ++;
    }

  mlx___vulkan_img_layout(frame->cmd_buff[0], vkwin->back_img,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  
  if (vkwin->clear_it)
    {
      clear_color.uint32[0] = 0;
      clear_color.uint32[1] = 0;
      clear_color.uint32[2] = 0;
      clear_color.uint32[3] = 255;
      clear_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      clear_range.baseMipLevel = 0;
      clear_range.levelCount = 1;
      clear_range.baseArrayLayer = 0;
      clear_range.layerCount = 1;
      vkCmdClearColorImage(frame->cmd_buff[0],
			   vkwin->back_img->image,
			   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			   &clear_color, 1, &clear_range);
      vkwin->clear_it = 0;
    }
  
  if (vkwin->draw_idx > 0) // no render pass if 0 image to draw
    {
      // assume render pass force initial & final layout
      render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      render_pass_info.pNext = NULL;
      render_pass_info.renderPass = vkwin->render_pass;
      render_pass_info.framebuffer = vkwin->back_img->img_fb;
      render_pass_info.renderArea.offset.x = 0;
      render_pass_info.renderArea.offset.y = 0;
      render_pass_info.renderArea.extent.width =
	vkwin->back_img->width;
      render_pass_info.renderArea.extent.height =
	vkwin->back_img->height;
      clear_value.color.float32[0] = 0.0;
      clear_value.color.float32[1] = 0.0;
      clear_value.color.float32[2] = 0.0;
      clear_value.color.float32[3] = 1.0;
      render_pass_info.clearValueCount = 1;
      render_pass_info.pClearValues = &clear_value;
      vkCmdBeginRenderPass(frame->cmd_buff[0], &(render_pass_info),
			   VK_SUBPASS_CONTENTS_INLINE);
      
      // same pipeline (==shaders) for all images
      vkCmdBindPipeline(frame->cmd_buff[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
			vkwin->pipeline);

      // loop through images to put and make a descriptor + draw each
      i = 0;
      while (i < vkwin->draw_idx)
	{
	  if (vkwin->draw_list[i].img_ref_idx >=0 &&
	      vk->img_ref[vkwin->draw_list[i].img_ref_idx].active)
	    {
	      bcopy(&(vkwin->draw_list[i].staging_uniform),
		    vkwin->draw_list[i].uniform,
		    sizeof(*(vkwin->draw_list[i].uniform)));
	      vkwin->draw_list[i].descriptor_set = frame->descriptor_set+i;
	      mlx___vulkan_desc_write(vk, vkwin->draw_list + i);
	      vkCmdBindDescriptorSets(frame->cmd_buff[0],
				      VK_PIPELINE_BIND_POINT_GRAPHICS,
				      vk->pipeline_layout, 0, 1,
				      frame->descriptor_set+i, 0, NULL);
	      vkCmdDraw(frame->cmd_buff[0], 6, 1, 0, 0);
	      img = (mlx___vulkan_img_t *)vk->img_ref[vkwin->draw_list[i].img_ref_idx].item;
	      img->staging_frame[vkwin->draw_list[i].img_staging_idx] = vkwin->cur_frame;
	      img->staging_status[vkwin->draw_list[i].img_staging_idx] = 2;
	    }
	  i ++;
	}
      vkwin->draw_idx = 0;
      
      vkCmdEndRenderPass(frame->cmd_buff[0]);
      // end of renderpass : back_img has a new layout
      vkwin->back_img->cur_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    }
  
  // finally copy rendered back_img to swap_chain for present
  if (vkwin->back_img->cur_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    mlx___vulkan_img_layout(frame->cmd_buff[0], vkwin->back_img,
			    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  mlx___vulkan_img_layout(frame->cmd_buff[0], vkwin->swch_images+swch_index,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vkCmdCopyImage(frame->cmd_buff[0], vkwin->back_img->image,
		 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		 vkwin->swch_images[swch_index].image,
		 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
		 &(vkwin->copy_region));
  mlx___vulkan_img_layout(frame->cmd_buff[0], vkwin->swch_images+swch_index,
			  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  
  if (vkEndCommandBuffer(frame->cmd_buff[0]) != VK_SUCCESS)
    return (1);

  return (0);
}

  
int	mlx___vulkan_draw_internal(mlx___vulkan_t *vk, mlx___vulkan_win_t *vkwin)
{
  mlx___vulkan_win_frame_t	*frame;
  uint32_t			sw_index;
  VkSubmitInfo			submit_info;
  VkPipelineStageFlags		wait_stages[2];
  VkPresentInfoKHR		present_info;
  int				prev_sema_idx;
  VkSemaphore			rp_wait_sema[2];
  VkSemaphore			rp_signal_sema[2];

  /*
  // wait for last frame, because last rendered image will be used.
  frame = vkwin->frames + (vkwin->cur_frame % VK_NB_FRAMES);
  vkWaitForFences(vk->vk_device, 1, frame->fence, VK_TRUE, UINT64_MAX);
  */
  if (vkwin->pixel_put_img_sync >= 0)
    {
      mlx___vulkan_img_mem_sync(vk, vkwin->pixel_put_img->data,
				vkwin->pixel_put_img->staging_data[vkwin->pixel_put_img_sync],
				vkwin->pixel_put_img->staging_device_memory[vkwin->pixel_put_img_sync],
				vkwin->pixel_put_img->size);
      vkwin->pixel_put_img_sync = -1;
    }
  
  vkwin->cur_frame ++;
  frame = vkwin->frames + (vkwin->cur_frame % VK_NB_FRAMES);

  prev_sema_idx = vkwin->swch_sema_idx;
  vkwin->swch_sema_idx = (vkwin->swch_sema_idx + 1) % (vkwin->swch_images_nb+1);
  
  if (vkAcquireNextImageKHR(vk->vk_device, vkwin->swap_chain, UINT64_MAX,
			    vkwin->swch_sema[3*vkwin->swch_sema_idx], VK_NULL_HANDLE, &sw_index) != VK_SUCCESS)
    {
      printf("draw: can't get next sw image\n");
      return (1);
    }
  if (vkwin->swch_fence[sw_index] != VK_NULL_HANDLE)
    vkWaitForFences(vk->vk_device, 1, vkwin->swch_fence+sw_index, VK_TRUE, UINT64_MAX);
  vkwin->swch_fence[sw_index] = frame->fence[0];

  vkWaitForFences(vk->vk_device, 1, frame->fence, VK_TRUE, UINT64_MAX);
  vkResetFences(vk->vk_device, 1, frame->fence);
  
  vkResetCommandBuffer(frame->cmd_buff[0], 0);
  mlx___vulkan_command_buffer_set_renderpass(vk, vkwin, sw_index);
  
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = NULL;
  submit_info.waitSemaphoreCount = 1;
  rp_wait_sema[0] = vkwin->swch_sema[3*vkwin->swch_sema_idx];
  if (prev_sema_idx >= 0)
    {
      submit_info.waitSemaphoreCount = 2;
      rp_wait_sema[1] = vkwin->swch_sema[3*prev_sema_idx + 2];
    }
  submit_info.pWaitSemaphores = rp_wait_sema;
  wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  wait_stages[1] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = frame->cmd_buff;
  submit_info.signalSemaphoreCount = 2;
  rp_signal_sema[0] = vkwin->swch_sema[3*vkwin->swch_sema_idx+1];
  rp_signal_sema[1] = vkwin->swch_sema[3*vkwin->swch_sema_idx+2];
  submit_info.pSignalSemaphores = rp_signal_sema;

  if (vkQueueSubmit(vk->vk_queue, 1, &submit_info, frame->fence[0]) !=
      VK_SUCCESS)
    return (1);

  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext = NULL;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = vkwin->swch_sema+3*vkwin->swch_sema_idx+1;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &(vkwin->swap_chain);
  present_info.pImageIndices = &sw_index;
  present_info.pResults = NULL;

  vkQueuePresentKHR(vk->vk_queue, &present_info);
  
  return (0);
}

int	mlx___vulkan_draw(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_t		*vk;
  mlx___vulkan_win_t		*vkwin;

  vk = (mlx___vulkan_t *)(param->gpu);
  vkwin = (mlx___vulkan_win_t *)(param->gpu_win);
  return (mlx___vulkan_draw_internal(vk, vkwin));
}



int	mlx___vulkan_sync_internal(mlx___vulkan_t *vk, mlx___vulkan_win_t *vkwin, mlx___vulkan_img_t *vkimg)
{
  mlx___vulkan_win_frame_t	*frame;
  int				i;
  mlx___vulkan_win_t		*win;

  if (vkwin)
    {
      // flush win, wait for last frame fence
      mlx___vulkan_draw_internal(vk, vkwin);
      frame = vkwin->frames + (vkwin->cur_frame % VK_NB_FRAMES);
      vkWaitForFences(vk->vk_device, 1, frame->fence, VK_TRUE, UINT64_MAX);
    }
  if (vkimg && vkimg->draw_win_ref_idx >= 0 && vk->win_ref[vkimg->draw_win_ref_idx].active)
    {
      win = (mlx___vulkan_win_t *)(vk->win_ref[vkimg->draw_win_ref_idx].item);
      i = VK_NB_FRAMES;
      while (i--)
		{
		  if (vkimg->staging_status[i] == 1)
			mlx___vulkan_draw_internal(vk, win); // will change status to 2
		  if (vkimg->staging_status[i] == 2)
			{
			  if (vkimg->staging_frame[i] > win->cur_frame - VK_NB_FRAMES)
				vkWaitForFences(vk->vk_device, 1, win->frames[vkimg->staging_frame[i]%VK_NB_FRAMES].fence, VK_TRUE, UINT64_MAX);
			  vkimg->staging_status[i] = 0;
			}
		}
    }
  return (0);
}


int	mlx___vulkan_sync(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_t		*vk;
  mlx___vulkan_win_t		*vkwin;
  mlx___vulkan_img_t		*vkimg;

  vk = (mlx___vulkan_t *)(param->gpu);
  vkwin = (mlx___vulkan_win_t *)(param->gpu_win);
  vkimg = (mlx___vulkan_img_t *)(param->gpu_img);
  if (vkwin == NULL && vkimg == NULL)
    return (1);
  return (mlx___vulkan_sync_internal(vk, vkwin, vkimg));
}
