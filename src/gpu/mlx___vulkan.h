

/* mlx___vulkan.h */

#ifndef MLX___VULKAN_H
#define MLX___VULKAN_H

void	*mlx___vulkan_init(mlx_gpu_hooks_param_t *param);
void	mlx___vulkan_destroy(mlx_gpu_hooks_param_t *param);
void    *mlx___vulkan_window(mlx_gpu_hooks_param_t *param);
void    *mlx___vulkan_window_clear(mlx_gpu_hooks_param_t *param);
void    *mlx___vulkan_window_pixel(mlx_gpu_hooks_param_t *param);
void    mlx___vulkan_window_destroy(mlx_gpu_hooks_param_t *param);
void    *mlx___vulkan_img_create(mlx_gpu_hooks_param_t *param);
int		mlx___vulkan_img_put(mlx_gpu_hooks_param_t *param);
void    mlx___vulkan_img_destroy(mlx_gpu_hooks_param_t *param);
int		mlx___vulkan_draw(mlx_gpu_hooks_param_t *param);
int		mlx___vulkan_sync(mlx_gpu_hooks_param_t *param);


#endif /* MLX___VULCAN_H */
