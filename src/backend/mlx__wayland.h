
/* mlx__wayland.h */

#ifndef MLX__WAYLAND_H
#define MLX__WAYLAND_H




/* external protos */

void	*mlx__wayland_init(mlx_backend_hooks_param_t *param);
void	mlx__wayland_destroy(mlx_backend_hooks_param_t *param);
void	*mlx__wayland_window(mlx_backend_hooks_param_t *param);
void	mlx__wayland_window_destroy(mlx_backend_hooks_param_t *param);
int	mlx__wayland_flush(mlx_backend_hooks_param_t *param);
int	mlx__wayland_event_hook(mlx_backend_hooks_param_t *param);
void	*mlx__wayland_event_get(mlx_backend_hooks_param_t *param);
void	mlx__wayland_event_handle(mlx_backend_hooks_param_t *param);
int	mlx__wayland_extra(mlx_backend_hooks_param_t *param);

#endif /* MLX__WAYLAND_H */
