
#include	<stdlib.h>
#include	<stdio.h>

#include	"mlx_internal.h"


int	mlx_loop(mlx_t *mlx)
{
  mlx_backend_hooks_param_t	bep;
  mlx_win_t			*win;

  bep.backend = mlx->backend;
  mlx->loop_ever = 1;
  
  while (mlx->loop_ever > 0)
    {
      mlx_do_sync(mlx); // flushes all windows
      if (mlx->loop_hook != NULL)
	{
	  bep.backend_event_block = 0;
	  mlx->loop_hook(mlx->loop_hook_param);
	}	  
      else
	bep.backend_event_block = 1;
      bep.event = MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].event_get, &bep);
      if (bep.event)
	{
	  win = mlx->windows;
	  while (win)
	    {
	      if (win->active)
		{
		  bep.backend_win = win->backend_win;
		  MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].event_handle, &bep);
		}
	      win = win->next;
	    }
	  bep.backend_win = NULL;
	  MLX_EXISTS_I(mlx_backend_hooks[mlx->backend_type].event_handle, &bep);
	  // to free event if needed
	}
    }
  if (mlx->loop_ever == -1)
    mlx_release(mlx);
  return (0);
}


int	mlx_loop_exit(mlx_t *mlx)
{
  mlx->loop_ever = 0;
  return (0);
}
