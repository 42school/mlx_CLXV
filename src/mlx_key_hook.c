
#include	<stdlib.h>

#include        "mlx_internal.h"


int	mlx_key_hook(mlx_win_t *win, int (*funct_ptr)(), void *param)
{
  mlx_backend_hooks_param_t	bep;

  bep.backend = win->mlx->backend;
  bep.backend_win = win->backend_win;
  bep.mlx_event = MLX_EVENT_KEY;
  bep.backend_event_hook = funct_ptr;
  bep.backend_event_param = param;
  return (MLX_EXISTS_I(mlx_backend_hooks[win->mlx->backend_type].event_hook,
		       &bep));
}
