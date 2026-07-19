
#include	<stdlib.h>

#include	"mlx_internal.h"

#if MLX_BACKEND == MLX_BACKEND_XCB
# include	"backend/mlx__xcb.h"
#elif MLX_BACKEND == MLX_BACKEND_WAYLAND
# include	"backend/mlx__wayland.h"
#endif
#include	"gpu/mlx___vulkan.h"


/* hooks index : MLX_BACKEND & MLX_GPU from mlx_config.h
   only one backend is ever compiled in (selected at build time via
   -DMLX_BACKEND, see the Makefile's BACKEND variable), designated
   initializers keep the array sized to whichever slot is actually used */

mlx_backend_hooks_t mlx_backend_hooks[] = {
#if MLX_BACKEND == MLX_BACKEND_XCB
  [MLX_BACKEND_XCB] =
  {mlx__xcb_init, mlx__xcb_destroy,
   mlx__xcb_window, NULL, mlx__xcb_window_destroy,
   NULL, NULL,
   mlx__xcb_event_hook, mlx__xcb_event_get, mlx__xcb_event_handle,
   mlx__xcb_flush, NULL,
   mlx__xcb_extra},
#elif MLX_BACKEND == MLX_BACKEND_WAYLAND
  [MLX_BACKEND_WAYLAND] =
  {mlx__wayland_init, mlx__wayland_destroy,
   mlx__wayland_window, NULL, mlx__wayland_window_destroy,
   NULL, NULL,
   mlx__wayland_event_hook, mlx__wayland_event_get, mlx__wayland_event_handle,
   mlx__wayland_flush, NULL,
   mlx__wayland_extra},
#endif
};

mlx_gpu_hooks_t mlx_gpu_hooks[] = {
  {mlx___vulkan_init, mlx___vulkan_destroy,
   mlx___vulkan_window, mlx___vulkan_window_clear, mlx___vulkan_window_pixel,
   mlx___vulkan_window_destroy,
   mlx___vulkan_img_create, mlx___vulkan_img_put, mlx___vulkan_img_destroy,
   NULL, mlx___vulkan_draw, mlx___vulkan_sync }
};
