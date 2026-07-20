
/* mlx__appkit.h */

#ifndef MLX__APPKIT_H
#define MLX__APPKIT_H




/* external protos */

void	*mlx__appkit_init(mlx_backend_hooks_param_t *param);
void	mlx__appkit_destroy(mlx_backend_hooks_param_t *param);
void	*mlx__appkit_window(mlx_backend_hooks_param_t *param);
void	mlx__appkit_window_destroy(mlx_backend_hooks_param_t *param);
int	mlx__appkit_flush(mlx_backend_hooks_param_t *param);
int	mlx__appkit_event_hook(mlx_backend_hooks_param_t *param);
void	*mlx__appkit_event_get(mlx_backend_hooks_param_t *param);
void	mlx__appkit_event_handle(mlx_backend_hooks_param_t *param);
int	mlx__appkit_extra(mlx_backend_hooks_param_t *param);

#endif /* MLX__APPKIT_H */
