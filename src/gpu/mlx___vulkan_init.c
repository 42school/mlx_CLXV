
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	"mlx_internal.h"

#include	<vulkan/vulkan.h>

#include	"mlx___vulkan_internal.h"
#include	"mlx___vulkan_shaders_code.h"


unsigned char mlx___vulkan_order[5] = { 4, 1, 0, 2, 3 };


static void	*mlx___vulkan_init_error(mlx___vulkan_t *mxvk,
					 char *msg, VkResult err)
{
  // Vulkan spec says vkDestroy* can be called with NULL handles
  if (msg)
    fprintf(stderr, "Mlx - Vk: %x - %s\n", (int)err, msg);
  if (mxvk)
    {
	  if (mxvk->vk_device)
		{
		  vkDeviceWaitIdle(mxvk->vk_device); // pas subtil ...
      
		  vkDestroyPipelineLayout(mxvk->vk_device, mxvk->pipeline_layout, NULL);
		  vkDestroyDescriptorSetLayout(mxvk->vk_device,
									   mxvk->descriptor_set_layout, NULL);
		  vkDestroyShaderModule(mxvk->vk_device, mxvk->shader_frag, NULL);
		  vkDestroyShaderModule(mxvk->vk_device, mxvk->shader_vert, NULL);
		  vkDestroyCommandPool(mxvk->vk_device, mxvk->command_pool, NULL);
		  vkDestroyDevice(mxvk->vk_device, NULL);
		}
	  if (mxvk->devices_prop)
		free(mxvk->devices_prop);
	  if (mxvk->devices)
		free(mxvk->devices);
	  if (mxvk->instance)
		vkDestroyInstance(mxvk->instance, NULL);
	  if (mxvk->dev_extensions)
		free(mxvk->dev_extensions);
	  if (mxvk->validation_layer)
		free(mxvk->validation_layer);
	  if (mxvk->inst_extensions)
		free(mxvk->inst_extensions);
	  if (mxvk->win_ref)
		free(mxvk->win_ref);
      if (mxvk->img_ref)
		free(mxvk->img_ref);
      free(mxvk);
    }
  return (NULL);
}


void	mlx___vulkan_destroy(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_t *mxvk;

  mxvk = (mlx___vulkan_t *)param->gpu;
  mlx___vulkan_init_error(mxvk, NULL, VK_SUCCESS);
}


static VkResult	mlx___vulkan_shaders(mlx___vulkan_t *vk)
{
  VkShaderModuleCreateInfo	shader_crea_info;
  VkResult			vkerr;

  shader_crea_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shader_crea_info.pNext = NULL;
  shader_crea_info.flags = 0;
  shader_crea_info.codeSize = shader_vert_spv_len;
  shader_crea_info.pCode = (uint32_t *)shader_vert_spv;
  if ((vkerr = vkCreateShaderModule(vk->vk_device, &shader_crea_info, NULL,
				    &(vk->shader_vert))) != VK_SUCCESS)
    return (vkerr);

  shader_crea_info.codeSize = shader_frag_spv_len;
  shader_crea_info.pCode = (uint32_t *)shader_frag_spv;
  return (vkCreateShaderModule(vk->vk_device, &shader_crea_info, NULL,
			       &(vk->shader_frag)));
}


static VkResult	mlx___vulkan_pipeline_layout(mlx___vulkan_t *vk)
{
  VkDescriptorSetLayoutCreateInfo	layout_create_info;
  VkPipelineLayoutCreateInfo		pl_crea_info;
  VkDescriptorSetLayoutBinding		layout_binding[2];
  VkResult				vkerr;

  // first layout: uniform
  layout_binding[0].binding = 0;
  layout_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layout_binding[0].descriptorCount = 1;
  layout_binding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  layout_binding[0].pImmutableSamplers = NULL;

  // second layout: texture
  layout_binding[1].binding = 1;
  layout_binding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  layout_binding[1].descriptorCount = 1;
  layout_binding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  layout_binding[1].pImmutableSamplers = NULL;

  // create descriptor set layout
  layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_create_info.pNext = NULL;
  layout_create_info.flags = 0;
  layout_create_info.bindingCount = 2;
  layout_create_info.pBindings = layout_binding;

  if ((vkerr = vkCreateDescriptorSetLayout(vk->vk_device, &layout_create_info,
					   NULL,
					   &(vk->descriptor_set_layout)))
      != VK_SUCCESS)
    return (vkerr);
 
  // then, create pipeline layout
  pl_crea_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pl_crea_info.pNext = NULL;
  pl_crea_info.flags = 0;
  pl_crea_info.setLayoutCount = 1;
  pl_crea_info.pSetLayouts = &(vk->descriptor_set_layout);
  pl_crea_info.pushConstantRangeCount = 0;
  pl_crea_info.pPushConstantRanges = NULL;
  
  return (vkCreatePipelineLayout(vk->vk_device, &pl_crea_info, NULL,
				 &(vk->pipeline_layout)));
}


int	mlx___vulkan_ref_img_add(mlx___vulkan_t *vk, mlx___vulkan_img_t *img)
{
  void	*ptr;
  
  if (vk->img_ref_count % VK_REF_CHUNK == 0)
    {
      ptr = reallocarray(vk->img_ref, (1+vk->img_ref_count/VK_REF_CHUNK)*VK_REF_CHUNK, sizeof(*(vk->img_ref)));
      if (ptr == NULL)
	return (-1);
      vk->img_ref = ptr;
      bzero(vk->img_ref+vk->img_ref_count, VK_REF_CHUNK * sizeof(*(vk->img_ref)));
    }
  vk->img_ref[vk->img_ref_count].active = 1;
  vk->img_ref[vk->img_ref_count].item = (void *)img;
  return (vk->img_ref_count++);
}

int	mlx___vulkan_ref_win_add(mlx___vulkan_t *vk, mlx___vulkan_win_t *win)
{
  void	*ptr;

  if (vk->win_ref_count % VK_REF_CHUNK == 0)
    {
      ptr = reallocarray(vk->win_ref, (1+vk->win_ref_count/VK_REF_CHUNK)*VK_REF_CHUNK, sizeof(*(vk->win_ref)));
      if (ptr == NULL)
	return (-1);
      vk->win_ref = ptr;
      bzero(vk->win_ref+vk->win_ref_count, VK_REF_CHUNK * sizeof(*(vk->win_ref)));
    }
  vk->win_ref[vk->win_ref_count].active = 1;
  vk->win_ref[vk->win_ref_count].item = (void *)win;
  return (vk->win_ref_count++);
}


int	mlx___vulkan_check_layers(const char **req_layers, int nb_req)
{
  uint32_t 			nb;
  VkLayerProperties *layers;
  int 				i;
  int				j;
  int				found;

  nb = 0;
  vkEnumerateInstanceLayerProperties(&nb, NULL);
  if ((layers = malloc(sizeof(VkLayerProperties) * nb)) == NULL)
	return (1);
  vkEnumerateInstanceLayerProperties(&nb, layers);

  i = nb_req;
  while (i--)
	{
	  found = 0;
	  j = nb;
	  while (j--)
		{
		  if (strcmp(req_layers[i], layers[j].layerName) == 0)
			found = 1;
        }
	  if (found == 0)
		{
		  free(layers);
		  return (1);
		}
    }
  free(layers);
  return (0);
}


int	mlx___vulkan_check_extensions(const char **req_ext, int nb_req)
{
  uint32_t				nb;
  VkExtensionProperties *ext;
  int 					i;
  int					j;
  int					found;

  nb = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &nb, NULL);
  if ((ext = malloc(sizeof(VkExtensionProperties) * nb)) == NULL)
	return (1);
  vkEnumerateInstanceExtensionProperties(NULL, &nb, ext);

  i = nb_req;
  while (i--)
	{
	  found = 0;
	  j = nb;
	  while (j--)
		{
		  if (strcmp(req_ext[i], ext[j].extensionName) == 0)
			found = 1;
        }
	  if (found == 0)
		{
		  free(ext);
		  return (1);
		}
    }
  free(ext);
  return (0);
}



void	*mlx___vulkan_init(mlx_gpu_hooks_param_t *param)
{
  mlx___vulkan_t		*mxvk;
  VkResult			vkerr;
  VkInstanceCreateInfo		inst_crea_info;
  VkDeviceCreateInfo		dev_crea_info;
  VkQueueFamilyProperties	*qfp;
  VkDeviceQueueCreateInfo	queue_info;
  VkCommandPoolCreateInfo	cmdpool_crea_info;
  unsigned int			qf_nb;
  int				qf_candidate;
  int				qf_final;
  int				nb_extensions;
  int				i;
  int				j;
  
  if ( (mxvk = malloc(sizeof(*mxvk))) == NULL )
    return (mlx___vulkan_init_error(mxvk, "can't malloc", 0));
  bzero(mxvk, sizeof(*mxvk));
  
  if ( (mxvk->inst_extensions = malloc(2 * sizeof(*mxvk->inst_extensions)))
       == NULL )
    return (mlx___vulkan_init_error(mxvk, "can't malloc", 0));
  if ( (mxvk->validation_layer = malloc(1 * sizeof(*mxvk->validation_layer)))
       == NULL )
    return (mlx___vulkan_init_error(mxvk, "can't malloc", 0));
  if ( (mxvk->dev_extensions = malloc(1 * sizeof(*mxvk->dev_extensions)))
       == NULL )
    return (mlx___vulkan_init_error(mxvk, "can't malloc", 0));

  mxvk->inst_extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
  nb_extensions = 1;
#if defined MLX_BACKEND && MLX_BACKEND == MLX_BACKEND_XCB
  mxvk->inst_extensions[1] = "VK_KHR_xcb_surface"; // VK_KHR_XCB_SURFACE_EXTENSION_NAME
  nb_extensions = 2;
#endif
  
  mxvk->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  mxvk->app_info.pNext = NULL;
  mxvk->app_info.pApplicationName = "mlx___vulkan";
  mxvk->app_info.applicationVersion = 1;
  mxvk->app_info.pEngineName = NULL;
  mxvk->app_info.engineVersion = 0;
  mxvk->app_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);

  /* create vk instance */
  inst_crea_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  inst_crea_info.pNext = NULL;
  inst_crea_info.flags = 0;
  inst_crea_info.pApplicationInfo = &(mxvk->app_info);
  inst_crea_info.enabledExtensionCount = nb_extensions;
  inst_crea_info.ppEnabledExtensionNames = mxvk->inst_extensions;
#ifdef VK_DEBUG_LAYER
  mxvk->validation_layer[0] = "VK_LAYER_KHRONOS_validation";
  inst_crea_info.enabledLayerCount = 1;
  inst_crea_info.ppEnabledLayerNames = mxvk->validation_layer;
#else
  inst_crea_info.enabledLayerCount = 0;
  inst_crea_info.ppEnabledLayerNames = NULL;
#endif

  if (mlx___vulkan_check_extensions(mxvk->inst_extensions, nb_extensions))
	return (mlx___vulkan_init_error(mxvk, "Missing required Vulkan extensions", VK_ERROR_UNKNOWN));

  if (mlx___vulkan_check_layers(mxvk->validation_layer, inst_crea_info.enabledLayerCount))
	return (mlx___vulkan_init_error(mxvk, "Missing Vulkan validation layers", VK_ERROR_UNKNOWN));
  
  if ((vkerr = vkCreateInstance(&inst_crea_info, NULL, &(mxvk->instance))) !=
      VK_SUCCESS)
    return (mlx___vulkan_init_error(mxvk, "vkCreateInstance", vkerr));

  /* after instance, get devices and queues */

  mxvk->devices_nb = 0;
  if ((vkerr = vkEnumeratePhysicalDevices(mxvk->instance, &(mxvk->devices_nb),
					  NULL)) != VK_SUCCESS ||
      mxvk->devices_nb < 1)
    return (mlx___vulkan_init_error(mxvk, "vkEnumPhysDevices", vkerr));
  //printf("vk phy devices nb %d\n", mxvk->devices_nb);
  if ((mxvk->devices = malloc(mxvk->devices_nb * sizeof(*(mxvk->devices)))) ==
      NULL)
    return (mlx___vulkan_init_error(mxvk, "can't malloc", 0));
  if ((mxvk->devices_prop = malloc(mxvk->devices_nb * sizeof(*(mxvk->devices_prop)))) == NULL)
    return (mlx___vulkan_init_error(mxvk, "can't malloc", 0));
  if ((vkerr = vkEnumeratePhysicalDevices(mxvk->instance, &(mxvk->devices_nb),
					  mxvk->devices)) != VK_SUCCESS)
    return (mlx___vulkan_init_error(mxvk, "vkEnumPhysDevices", vkerr));
  i = 0;
  //printf("nb vulkan devices : %d\n", mxvk->devices_nb);
  mxvk->dev = 0; // at least there is one
  qf_final = -1;
  while (i < mxvk->devices_nb)
    {
      vkGetPhysicalDeviceProperties(mxvk->devices[i], mxvk->devices_prop+i);
      /* printf(" %d : %d - %s - V. %d.%d.%d\n", i, (mxvk->devices_prop+i)->deviceType, (mxvk->devices_prop+i)->deviceName,
	     VK_VER_MAJOR((mxvk->devices_prop+i)->apiVersion),
	     VK_VER_MINOR((mxvk->devices_prop+i)->apiVersion),
	     VK_VER_PATCH((mxvk->devices_prop+i)->apiVersion)
	     ); */
      qf_nb = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(mxvk->devices[i], &qf_nb, NULL);
      if ((qfp = malloc(qf_nb * sizeof(*qfp))) == NULL)
	return (mlx___vulkan_init_error(mxvk, "can't malloc", 0));
      vkGetPhysicalDeviceQueueFamilyProperties(mxvk->devices[i], &qf_nb, qfp);
      //printf("  nb queues families: %d\n", qf_nb);
      qf_candidate = -1;
      j = qf_nb;
      while (j --)
	{
	  /*
	    printf("   %d: Gr %d, Cp %d, Tr %d, Sp %d\n", j, (qfp+j)->queueFlags & VK_QUEUE_GRAPHICS_BIT,
		 (qfp+j)->queueFlags & VK_QUEUE_COMPUTE_BIT, (qfp+j)->queueFlags & VK_QUEUE_TRANSFER_BIT,
		 (qfp+j)->queueFlags & VK_QUEUE_SPARSE_BINDING_BIT); */
	  if ((qfp+j)->queueFlags & (VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_TRANSFER_BIT))
	    qf_candidate = j;
	}
      free(qfp);
      
      if (mlx___vulkan_order[(mxvk->devices_prop+i)->deviceType] <=
	  mlx___vulkan_order[(mxvk->devices_prop+mxvk->dev)->deviceType] &&
	  qf_candidate >= 0)
	{
	  mxvk->dev = i;
	  qf_final = qf_candidate;
	}
      i ++;
    }

  if (qf_final < 0)
    return (mlx___vulkan_init_error(mxvk, "no available Queue", 0));

  // printf("=> selected device id %d and queue family %d\n", mxvk->dev, qf_final);

  mxvk->queue_priorities = 1.0;
  bzero(&queue_info, sizeof(queue_info));
  queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info.pNext = NULL;
  queue_info.queueFamilyIndex = qf_final;
  queue_info.flags = 0;
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = &(mxvk->queue_priorities);

  mxvk->dev_extensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

  bzero(&dev_crea_info, sizeof(dev_crea_info));
  dev_crea_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dev_crea_info.pNext = NULL;
  dev_crea_info.flags = 0;
  dev_crea_info.queueCreateInfoCount = 1;
  dev_crea_info.pQueueCreateInfos = &queue_info;
  dev_crea_info.enabledExtensionCount = 1;
  dev_crea_info.ppEnabledExtensionNames = mxvk->dev_extensions;
  dev_crea_info.pEnabledFeatures = NULL;

  if ((vkerr = vkCreateDevice(mxvk->devices[mxvk->dev], &dev_crea_info,
			      NULL, &(mxvk->vk_device))) != VK_SUCCESS)
    return (mlx___vulkan_init_error(mxvk, "vkCreateDevice", vkerr));

  // printf("Ok, vk device created.\n");
  
  /* queue */
  vkGetDeviceQueue(mxvk->vk_device, qf_final, 0, &(mxvk->vk_queue));

  /* physical device mem properties */
  vkGetPhysicalDeviceMemoryProperties(mxvk->devices[mxvk->dev],
				      &mxvk->device_mem_prop);
  
  /* command pool */
  bzero(&cmdpool_crea_info, sizeof(cmdpool_crea_info));
  cmdpool_crea_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdpool_crea_info.pNext = NULL;
  cmdpool_crea_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cmdpool_crea_info.queueFamilyIndex = qf_final;

  if ((vkerr = vkCreateCommandPool(mxvk->vk_device, &cmdpool_crea_info,
				   NULL, &(mxvk->command_pool))) != VK_SUCCESS)
    return (mlx___vulkan_init_error(mxvk, "vkCreateCmdPool", vkerr));

  /* shaders */
  if ((vkerr = mlx___vulkan_shaders(mxvk)) != VK_SUCCESS)
    return (mlx___vulkan_init_error(mxvk, "shaders", vkerr));

  /* pipeline layout */
  if ((vkerr = mlx___vulkan_pipeline_layout(mxvk)) != VK_SUCCESS)
    return (mlx___vulkan_init_error(mxvk, "pipeline layout", vkerr));

  return (mxvk);
}
