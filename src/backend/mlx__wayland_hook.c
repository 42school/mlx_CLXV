
#include	<stdlib.h>

#include	"mlx_internal.h"

#include	<wayland-client.h>

#include	"mlx__wayland_internal.h"


/*
** mlx_event enum (mlx_internal.h) -> our own backend event numbers.
** Unlike XCB there is no event mask to maintain: Wayland always
** delivers pointer/keyboard events once the objects are bound, masking
** is purely a matter of whether a hook is registered or not.
*/
static int	mlx__wayland_convert_event[4] =
  {
    0,
    MLX_WL_EVENT_KEY_RELEASE,   /* MLX_EVENT_KEY - fired on release, like XCB */
    MLX_WL_EVENT_BUTTON_PRESS,  /* MLX_EVENT_MOUSE */
    MLX_WL_EVENT_EXPOSE         /* MLX_EVENT_EXPOSE */
  };


int	mlx__wayland_event_hook(mlx_backend_hooks_param_t *param)
{
  mlx__wayland_win_t	*win;

  win = (mlx__wayland_win_t *)(param->backend_win);
  if (param->mlx_event > 0)
    param->backend_event_number = mlx__wayland_convert_event[param->mlx_event];
  if (param->backend_event_number < 0 ||
      param->backend_event_number >= MLX_WAYLAND_MAX_EVENT)
    return (-1);
  win->hook[param->backend_event_number] = param->backend_event_hook;
  win->hook_param[param->backend_event_number] = param->backend_event_param;
  return (0);
}
