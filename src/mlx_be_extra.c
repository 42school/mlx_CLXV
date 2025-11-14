
#include	"mlx_internal.h"


int     mlx_mouse_hide(mlx_t *mlx)
{
  mlx_backend_hooks_param_t	bep;
  mlx_win_t			*win;

  bep.backend = mlx->backend;
  bep.action = MLX_BE_MOUSE_HIDE;
  win = mlx->windows;
  while (win)
    {
      if (win->active)
	{
	  bep.backend_win = win->backend_win;
	  MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].extra, &bep);
	}
      win = win->next;
    }
  return (0);
}

int     mlx_mouse_show(mlx_t *mlx)
{
  mlx_backend_hooks_param_t	bep;
  mlx_win_t			*win;

  bep.backend = mlx->backend;
  bep.action = MLX_BE_MOUSE_SHOW;
  win = mlx->windows;
  while (win)
    {
      if (win->active)
	{
	  bep.backend_win = win->backend_win;
	  MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].extra, &bep);
	}
      win = win->next;
    }
  return (0);
}

int     mlx_mouse_move(mlx_win_t *win, int x, int y)
{
  mlx_backend_hooks_param_t	bep;

  bep.backend = win->mlx->backend;
  bep.backend_win = win->backend_win;
  bep.action = MLX_BE_MOUSE_SET;
  bep.size.x = x;
  bep.size.y = y;
  return (MLX_EXISTS_I(mlx_backend_hooks[win->mlx->backend_type].extra, &bep));
}

int     mlx_mouse_get_pos(mlx_win_t *win, int *x, int *y)
{
  mlx_backend_hooks_param_t	bep;

  bep.backend = win->mlx->backend;
  bep.backend_win = win->backend_win;
  bep.action = MLX_BE_MOUSE_GET;
  if (MLX_EXISTS_I(mlx_backend_hooks[win->mlx->backend_type].extra, &bep) == 0)
    {
      *x = bep.size.x;
      *y = bep.size.y;
      return (0);
    }
  return (-1);
}

int	mlx_do_key_autorepeatoff(mlx_t *mlx)
{
  mlx_backend_hooks_param_t	bep;

  bep.backend = mlx->backend;
  bep.action = MLX_BE_KEY_AUTO_OFF;
  return (MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].extra, &bep));
}

int	mlx_do_key_autorepeaton(mlx_t *mlx)
{
  mlx_backend_hooks_param_t	bep;

  bep.backend = mlx->backend;
  bep.action = MLX_BE_KEY_AUTO_ON;
  return (MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].extra, &bep));
}
