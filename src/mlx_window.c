
#include	<stdlib.h>
#include	<stdint.h>

#include	"mlx_internal.h"


static void	*mlx_window_error(mlx_t *mlx, mlx_win_t *win)
{
  mlx_backend_hooks_param_t	bep;
  mlx_gpu_hooks_param_t		gpup;
  mlx_win_t			prev_win;
  mlx_win_t			*win_chain;

  if (win)
    {
      if (win->gpu_win)
	{
	  gpup.gpu = mlx->gpu;
	  gpup.gpu_win = win->gpu_win;
	  MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].window_release, &gpup);
	  win->gpu_win = NULL;
	}
      if (win->backend_win)
	{
	  bep.backend = mlx->backend;
	  bep.backend_win = win->backend_win;
	  MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].window_release,
		       &bep);
	  win->backend_win = NULL;
	}
      win->active = 0;

      if (mlx->loop_ever <= 0)  // remove from list & free only outside mlx_loop
	{
	  prev_win.next = mlx->windows;
	  win_chain = &prev_win;
	  while (win_chain && win_chain->next != win)
	    win_chain = win_chain->next;
	  if (win_chain)
	    win_chain->next = win->next;
	  mlx->windows = prev_win.next;
	  free(win);
	}
    }
  return (NULL);
}

void	*mlx_new_window(mlx_t *mlx, unsigned int width, unsigned int height,
			const char *title)
{
  mlx_win_t			*win;
  mlx_backend_hooks_param_t	bep;
  mlx_gpu_hooks_param_t		gpup;

  if ( (win = malloc(sizeof(*win))) == NULL )
      return (NULL);
  win->mlx = mlx;
  win->backend_win = NULL;
  win->gpu_win = NULL;

  bep.size.width = width;
  bep.size.height = height;
  bep.title = (char *)title;
  bep.backend = mlx->backend;
  win->backend_win =
    MLX_EXISTS_P(mlx_backend_hooks[mlx->backend_type].window, &bep);
  if (win->backend_win == NULL)
    return (mlx_window_error(mlx, win));
  
  gpup.gpu = mlx->gpu;
  gpup.dst.width = width;
  gpup.dst.height = height;
  gpup.backend = mlx->backend;
  gpup.backend_win = win->backend_win;
  win->gpu_win = MLX_EXISTS_P(mlx_gpu_hooks[mlx->gpu_type].window, &gpup);
  if (win->gpu_win == NULL)
    return (mlx_window_error(mlx, win));
  
  win->next = mlx->windows;
  mlx->windows = win;
  win->active = 1;
  
  return (win);
}


int	mlx_clear_window(mlx_t *mlx, mlx_win_t *win)
{
  mlx_backend_hooks_param_t	bep;
  mlx_gpu_hooks_param_t		gpup;

  bep.backend = mlx->backend;
  bep.backend_win = win->backend_win;
  MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].window_clear, &bep);
  
  gpup.gpu = mlx->gpu;
  gpup.gpu_win = win->gpu_win;
  MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].window_clear, &gpup);

  return (0);
}


int	mlx_destroy_window(mlx_t *mlx, mlx_win_t *win)
{
  mlx_window_error(mlx, win);
  return (0);
}


int	mlx_pixel_put(mlx_t *mlx, mlx_win_t *win,
		      unsigned int x, unsigned int y, unsigned int color)
{
  mlx_gpu_hooks_param_t		gpup;

  gpup.gpu = mlx->gpu;
  gpup.gpu_win = win->gpu_win;
  gpup.dst.x = x;
  gpup.dst.y = y;
  gpup.color = color;
  MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].window_pixel, &gpup);
  return (0);
}
