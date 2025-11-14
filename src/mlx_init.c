
#include	<stdlib.h>
#include	<stdio.h>
#include	<strings.h>

#include	"mlx_internal.h"


static void	*mlx_error(mlx_t *mlx, char *msg)
{
  mlx_backend_hooks_param_t bep;
  mlx_gpu_hooks_param_t gpup;

  if (msg)
    fprintf(stderr, "Mlx init: %s\n", msg);
  if (mlx)
    {
      gpup.gpu = mlx->gpu;
      bep.backend = mlx->backend;
      if (mlx->gpu)
		MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].release, &gpup);
      if (mlx->backend)
		MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].release, &bep);
      free(mlx);
    }
  return (NULL);
}


int	mlx_release(mlx_t *mlx)
{
  mlx_win_t	*win;
  mlx_win_t	*win_next;

  if (mlx->loop_ever > 0)
    {
      mlx->loop_ever = -1;
      return (0);
    }
  win = mlx->windows;
  while (win)
    {
      win_next = win->next;
      free(win);
      win = win_next;
    }
  mlx_string_put(mlx, NULL, 0, 0, 0, NULL); // release string image
  mlx_error(mlx, NULL);
  return (0);
}


void	*mlx_init()
{
  mlx_t *mlx;
  mlx_backend_hooks_param_t be;
  
  if ((mlx = malloc(sizeof(*mlx))) == NULL)
    return (NULL);
  bzero(mlx, sizeof(*mlx));
  mlx->windows = NULL;
  mlx->backend_type = MLX_BACKEND;
  mlx->gpu_type = MLX_GPU;
  mlx->backend = MLX_EXISTS_P(mlx_backend_hooks[mlx->backend_type].init, &be);
  if (mlx->backend == NULL)
	return (mlx_error(mlx, "Can't initialize backend"));
  mlx->screen_width = be.size.width;
  mlx->screen_height = be.size.height;
  mlx->gpu = MLX_EXISTS_P(mlx_gpu_hooks[mlx->gpu_type].init, NULL);
  mlx->loop_hook = NULL;
  mlx->loop_hook_param = NULL;
  mlx->loop_ever = 0;
  if (mlx->gpu == NULL)
	return (mlx_error(mlx, "Can't initialize gpu"));
  return (mlx);
}


int	mlx_get_screen_size(mlx_t *mlx_ptr,
			    unsigned int *width, unsigned int *height)
{
  *width = mlx_ptr->screen_width;
  *height = mlx_ptr->screen_height;
  return (0);
}
