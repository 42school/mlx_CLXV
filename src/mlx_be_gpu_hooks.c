
#include	<stdlib.h>

#include	"mlx_internal.h"

#include	"backend/mlx__xcb.h"
#include	"gpu/mlx___vulkan.h"


/* hooks index : MLX_BACKEND & MLX_GPU from mlx_config.h */

mlx_backend_hooks_t mlx_backend_hooks[] = {
  {mlx__xcb_init, mlx__xcb_destroy,
   mlx__xcb_window, NULL, mlx__xcb_window_destroy,
   NULL, NULL,
   mlx__xcb_event_hook, mlx__xcb_event_get, mlx__xcb_event_handle,
   mlx__xcb_flush, NULL,
   mlx__xcb_extra}
};

mlx_gpu_hooks_t mlx_gpu_hooks[] = {
  {mlx___vulkan_init, mlx___vulkan_destroy,
   mlx___vulkan_window, mlx___vulkan_window_clear, mlx___vulkan_window_pixel,
   mlx___vulkan_window_destroy,
   mlx___vulkan_img_create, mlx___vulkan_img_put, mlx___vulkan_img_destroy,
   NULL, mlx___vulkan_draw, mlx___vulkan_sync }
};
