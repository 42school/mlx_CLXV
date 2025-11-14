
#include	<stdlib.h>

#include        "mlx_internal.h"



int	mlx_hook(mlx_win_t *win, int x_event, int x_mask,
		 int (*funct_ptr)(), void *param)
{
  mlx_backend_hooks_param_t	bep;

  bep.backend = win->mlx->backend;
  bep.backend_win = win->backend_win;
  bep.mlx_event = MLX_EVENT_NONE;
  bep.backend_event_number = x_event;
  bep.backend_event_mask = x_mask;
  bep.backend_event_hook = funct_ptr;
  bep.backend_event_param = param;
  return (MLX_EXISTS_I(mlx_backend_hooks[win->mlx->backend_type].event_hook,
		       &bep));
}
