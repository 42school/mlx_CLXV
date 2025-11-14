

/* mlx_internal.h */

#ifndef MLX_INTERNAL_H
#define MLX_INTERNAL_H

#include	"mlx_config.h"

#ifndef NULL
#define	NULL	((void *)0)
#endif

#define	MLX_EVENT_NONE		0
#define	MLX_EVENT_KEY		1
#define	MLX_EVENT_MOUSE		2
#define	MLX_EVENT_EXPOSE	3

#define	MLX_BE_MOUSE_HIDE	1
#define	MLX_BE_MOUSE_SHOW	2
#define	MLX_BE_MOUSE_SET	3
#define	MLX_BE_MOUSE_GET	4
#define	MLX_BE_KEY_AUTO_OFF	5
#define	MLX_BE_KEY_AUTO_ON	6

#define MLX_SYNC_IMAGE_WRITABLE         1
#define MLX_SYNC_WIN_FLUSH              2
#define MLX_SYNC_WIN_COMPLETED          3


typedef	struct			mlx_xywh_s
{
  int	x;
  int	y;
  unsigned int	width;
  unsigned int	height;
} mlx_xywh_t;


typedef struct			mlx_backend_hooks_param_s
{
  void		*backend;
  void		*backend_win;
  void		*backend_img;
  mlx_xywh_t	size;
  char		*title;
  int		mlx_event;
  int		backend_event_number;
  int		backend_event_mask;
  int		(*backend_event_hook)();
  void		*backend_event_param;
  int		backend_event_block;
  void		*event;
  int		color;
  int		action;
} mlx_backend_hooks_param_t;



typedef struct		mlx_backend_hooks_s
{
  void			*(*init)(mlx_backend_hooks_param_t *param);
  void			(*release)(mlx_backend_hooks_param_t *param);
  void			*(*window)(mlx_backend_hooks_param_t *param);
  void			*(*window_clear)(mlx_backend_hooks_param_t *param);
  void			(*window_release)(mlx_backend_hooks_param_t *param);
  void			*(*image)(mlx_backend_hooks_param_t *param);
  void			(*image_release)(mlx_backend_hooks_param_t *param);
  int			(*event_hook)(mlx_backend_hooks_param_t *param);
  void			*(*event_get)(mlx_backend_hooks_param_t *param);
  void			(*event_handle)(mlx_backend_hooks_param_t *param);
  int			(*flush)(mlx_backend_hooks_param_t *param);
  int			(*flush_window)(mlx_backend_hooks_param_t *param);
  int			(*extra)(mlx_backend_hooks_param_t *param);
} mlx_backend_hooks_t;


typedef struct	mlx_gpu_hooks_param_s
{
  void		*gpu;
  void		*gpu_win;
  void		*gpu_img;
  mlx_xywh_t	src;
  mlx_xywh_t	dst;
  unsigned int	color;
  void		*backend;
  void		*backend_win;
  void		*data;
  unsigned int	format;
  unsigned int	sizeline;
} mlx_gpu_hooks_param_t;


typedef struct		mlx_gpu_hooks_s
{
  void			*(*init)(mlx_gpu_hooks_param_t *param);
  void			(*release)(mlx_gpu_hooks_param_t *param);
  void			*(*window)(mlx_gpu_hooks_param_t *param);
  void			*(*window_clear)(mlx_gpu_hooks_param_t *param);
  void			*(*window_pixel)(mlx_gpu_hooks_param_t *param);
  void			(*window_release)(mlx_gpu_hooks_param_t *param);
  void			*(*image)(mlx_gpu_hooks_param_t *param);
  int			(*image_put)(mlx_gpu_hooks_param_t *param);
  void			(*image_release)(mlx_gpu_hooks_param_t *param);
  int			(*flush)(mlx_gpu_hooks_param_t *param);
  int			(*flush_window)(mlx_gpu_hooks_param_t *param);
  int			(*sync_window)(mlx_gpu_hooks_param_t *param);
} mlx_gpu_hooks_t;


extern mlx_backend_hooks_t	mlx_backend_hooks[];
extern mlx_gpu_hooks_t		mlx_gpu_hooks[];


#define MLX_EXISTS_P(F,P)	((F)?(F)(P):NULL)
#define MLX_EXISTS_I(F,P)	((F)?(F)(P):0)



/* generic mlx structs */


typedef struct		mlx_win_s
{
  struct mlx_win_s	*next;
  int			active;
  struct mlx_s		*mlx;     /* needed for mouse/key hook: only get win param and not mlx param */
  void			*backend_win;
  void			*gpu_win;
} mlx_win_t;


typedef struct		mlx_img_s
{
  struct mlx_s		*mlx;
  void			*gpu_img;
  void			*data;
  unsigned int		format;
  unsigned int		sizeline;
} mlx_img_t;


typedef struct	mlx_s
{
  int		backend_type;
  void		*backend;
  int		gpu_type;
  void		*gpu;
  mlx_win_t	*windows;
  int		(*loop_hook)(void *user_param);
  void		*loop_hook_param;
  int		loop_ever;
  unsigned int	screen_width;
  unsigned int	screen_height;
} mlx_t;


/* Prototype for some public functions needed internally */
int		mlx_do_sync(mlx_t *mlx);
void		*mlx_new_image(mlx_t *mlx_ptr, unsigned int width,
			       unsigned int height);
unsigned char	*mlx_get_data_addr(mlx_img_t *img_ptr,
				   unsigned int *bits_per_pixel,
				   unsigned int *size_line,
				   unsigned int *format);
int		mlx_string_put(void *mlx_ptr, void *win_ptr,
			       int x, int y, int color, char *string);
void		mlx_destroy_image(mlx_t *mlx, mlx_img_t *img);
int		mlx_release(mlx_t *mlx);

#endif /* MLX_INTERNAL_H */
