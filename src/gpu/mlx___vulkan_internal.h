

/* mlx___vulkan_internal.h */

#ifndef MLX___VULKAN_INTERNAL_H
#define MLX___VULKAN_INTERNAL_H

#define VK_VER_MAJOR(X) ((((uint32_t)(X))>>22)&0x3FF)
#define VK_VER_MINOR(X) ((((uint32_t)(X))>>12)&0x3FF)
#define VK_VER_PATCH(X) (((uint32_t)(X)) & 0xFFF)

#define	MIN(X,Y)	((X)<=(Y)?(X):(Y))
#define	MAX(X,Y)	((X)>=(Y)?(X):(Y))

#define	VK_NB_FRAMES	3
#define VK_NB_DRAW	64

#define	VK_REF_CHUNK	128

typedef struct		mlx___vulkan_ref_s
{
  int				active;
  void				*item;
} mlx___vulkan_ref_t;


typedef struct						mlx___vulkan_s
{
  VkInstance						instance;
  VkApplicationInfo					app_info;
  const char						**inst_extensions;
  const char						**validation_layer;
  unsigned int						devices_nb;
  VkPhysicalDevice					*devices;
  VkPhysicalDeviceProperties		*devices_prop;
  int								dev;
  float								queue_priorities;
  const char						**dev_extensions;
  VkPhysicalDeviceMemoryProperties	device_mem_prop;
  VkDevice							vk_device;
  VkQueue							vk_queue;
  VkCommandPool						command_pool;
  VkShaderModule					shader_vert;
  VkShaderModule					shader_frag;
  VkPipelineLayout					pipeline_layout;
  VkDescriptorSetLayout				descriptor_set_layout;
  int								win_ref_count;
  int								img_ref_count;
  mlx___vulkan_ref_t				*win_ref;
  mlx___vulkan_ref_t				*img_ref;
} mlx___vulkan_t;


typedef struct		mlx___vulkan_img_uniform_s
{
  float				src_full[2];
  float				src_pos[2];
  float				src_size[2];
  float				dst_full[2];
  float				dst_pos[2];
  float				dst_size[2];
  float				color[4];
} mlx___vulkan_img_uniform_t;

typedef struct		mlx___vulkan_img_s
{
  int				ref_idx;
  VkImage			image;
  uint				width;
  uint				height;
  VkFormat			format;
  VkDeviceSize		size;
  VkDeviceMemory	img_device_memory;
  int				draw_win_ref_idx;
  void				*data;
  VkBuffer			staging_buffer[VK_NB_FRAMES];
  VkDeviceMemory	staging_device_memory[VK_NB_FRAMES];
  void				*staging_data[VK_NB_FRAMES];
  unsigned int		staging_frame[VK_NB_FRAMES];
  unsigned int		staging_status[VK_NB_FRAMES];
  VkImageView		image_view;
  VkSampler			texture_sampler;
  VkImageLayout		cur_layout;
  VkFramebuffer		img_fb;
  mlx___vulkan_img_uniform_t	default_uniform;
} mlx___vulkan_img_t;


typedef struct		mlx___vulkan_win_frame_s
{
  VkCommandBuffer	cmd_buff[2];
  VkFence			fence[1];
  VkDescriptorSet	descriptor_set[VK_NB_DRAW];
} mlx___vulkan_win_frame_t;

typedef struct		mlx___vulkan_draw_list_s
{
  VkDescriptorSet	*descriptor_set;
  VkBuffer			uniform_buffer;
  VkDeviceMemory	uniform_device_memory;
  mlx___vulkan_img_uniform_t	*uniform;
  mlx___vulkan_img_uniform_t	staging_uniform;
  int				img_ref_idx;
  int				img_staging_idx;
} mlx___vulkan_draw_list_t;


typedef struct			mlx___vulkan_win_s
{
  mlx___vulkan_t		*vk;
  int					ref_idx;
  VkSurfaceKHR			surface;
  unsigned int			sfmt_nb;
  VkSurfaceFormatKHR	*surf_fmt;
  VkFormat				format;
  unsigned int			width;
  unsigned int			height;
  VkColorSpaceKHR		color_space;
  VkSurfaceCapabilitiesKHR	surf_cap;
  VkSwapchainKHR		swap_chain;
  unsigned int			swch_images_nb;
  mlx___vulkan_img_t	*swch_images;
  VkFence				*swch_fence;
  VkSemaphore			*swch_sema;
  int					swch_sema_idx;
  mlx___vulkan_img_t	*back_img;
  VkImageCopy			copy_region;
  mlx___vulkan_img_t	*pixel_put_img;
  int					pixel_put_img_sync;
  VkDescriptorPool		descriptor_pool;
  mlx___vulkan_draw_list_t	draw_list[VK_NB_DRAW];
  unsigned int			draw_idx;
  int					clear_it;
  VkRenderPass			render_pass;
  VkPipeline			pipeline;
  unsigned int			cur_frame;
  mlx___vulkan_win_frame_t	frames[VK_NB_FRAMES];
  
} mlx___vulkan_win_t;



/* Internal Prototypes */

VkResult	mlx___vulkan_img_view(mlx___vulkan_t *vk,
				      mlx___vulkan_img_t *img);
int			mlx___vulkan_all_command_buffers(mlx___vulkan_t *vk,
						 mlx___vulkan_win_t *vkwin);
int			mlx___vulkan_command_buffer(mlx___vulkan_t *vk,
					    mlx___vulkan_win_t *vkwin,
					    int swch_index);
void		*mlx___vulkan_img_error(mlx___vulkan_t *vk,
					mlx___vulkan_img_t *img,
					char *msg, VkResult err);
void		*mlx___vulkan_img_create_internal(mlx___vulkan_t *vk,
						  uint width, uint height);
int			mlx___vulkan_draw_internal(mlx___vulkan_t *vk,
					   mlx___vulkan_win_t *vkwin);
int			mlx___vulkan_img_put_internal(mlx___vulkan_t *vk,
					      mlx___vulkan_win_t *vkwin,
					      mlx___vulkan_img_t *vkimg,
					      mlx___vulkan_img_uniform_t *uniform);
VkResult	mlx___vulkan_img_layout(VkCommandBuffer cmd_buffer,
					mlx___vulkan_img_t *vkimg,
					VkImageLayout to_layout);
int			mlx___vulkan_sync_internal(mlx___vulkan_t *vk,
					   mlx___vulkan_win_t *vkwin,
					   mlx___vulkan_img_t *vkimg);
uint32_t	mlx___vulkan_get_mem_type(mlx___vulkan_t *vk, uint32_t filter,
					  VkMemoryPropertyFlags properties);
VkResult	mlx___vulkan_create_buffer(mlx___vulkan_t *vk,
					   VkDeviceSize size,
					   VkBufferUsageFlags buff_usage,
					   VkMemoryPropertyFlags mem_properties,
					   VkBuffer *vk_buffer,
					   VkDeviceMemory *buffer_memory);
int			mlx___vulkan_ref_img_add(mlx___vulkan_t *vk, mlx___vulkan_img_t *img);
int			mlx___vulkan_ref_win_add(mlx___vulkan_t *vk, mlx___vulkan_win_t *win);
void		mlx___vulkan_img_mem_sync(mlx___vulkan_t *vk, void *src, void *dst, VkDeviceMemory dst_gpu, VkDeviceSize size);


#endif /* MLX___VULCAN_INTERNAL_H */
