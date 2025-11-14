
/* mlx__xcb.h */

#ifndef MLX__XCB_H
#define MLX__XCB_H




/* external protos */

void	*mlx__xcb_init(mlx_backend_hooks_param_t *param);
void	mlx__xcb_destroy(mlx_backend_hooks_param_t *param);
void	*mlx__xcb_window(mlx_backend_hooks_param_t *param);
void    mlx__xcb_window_destroy(mlx_backend_hooks_param_t *param);
int	mlx__xcb_flush(mlx_backend_hooks_param_t *param);
int	mlx__xcb_event_hook(mlx_backend_hooks_param_t *param);
void	*mlx__xcb_event_get(mlx_backend_hooks_param_t *param);
void	mlx__xcb_event_handle(mlx_backend_hooks_param_t *param);
int	mlx__xcb_extra(mlx_backend_hooks_param_t *param);

#endif /* MLX__XCB_H */
