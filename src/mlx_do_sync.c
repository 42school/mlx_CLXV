
#include	<stdlib.h>

#include	"mlx_internal.h"


/* actually flushes only */
int	mlx_do_sync(mlx_t *mlx)
{
  mlx_win_t			*win;
  mlx_backend_hooks_param_t	bep;
  mlx_gpu_hooks_param_t		gpup;

  bep.backend = mlx->backend;
  gpup.gpu = mlx->gpu;
  MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].flush, &bep);
  MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].flush, &gpup);

  win = mlx->windows;
  while (win)
    {
      if (win->active)
	{
	  bep.backend_win = win->backend_win;
	  gpup.gpu_win = win->gpu_win;
	  MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].flush_window, &bep);
	  MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].flush_window, &gpup);
	}
      win = win->next;
    }
  return (0);
}


int	mlx_sync(mlx_t *mlx, int cmd, void *param)
{
  mlx_win_t			*win;
  mlx_img_t			*img;
  mlx_backend_hooks_param_t	bep;
  mlx_gpu_hooks_param_t		gpup;

  bep.backend = mlx->backend;
  gpup.gpu = mlx->gpu;

  if (cmd == MLX_SYNC_WIN_FLUSH)
    {
      win = (mlx_win_t *)param;
      bep.backend_win = win->backend_win;
      gpup.gpu_win = win->gpu_win;
      MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].flush_window, &bep);
      MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].flush_window, &gpup);
      return (0);
    }

  if (cmd == MLX_SYNC_WIN_COMPLETED)
    {
      win = (mlx_win_t *)param;
      bep.backend_win = win->backend_win;
      gpup.gpu_win = win->gpu_win;
      gpup.gpu_img = NULL;
      MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].sync_window, &gpup);
      return (0);
    }
  
  if (cmd == MLX_SYNC_IMAGE_WRITABLE)
    {
      img = (mlx_img_t *)param;
      gpup.gpu_img = img->gpu_img;
      gpup.gpu_win = NULL;
      MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].sync_window, &gpup);
      return (0);
    }
  
  return (1);
}
