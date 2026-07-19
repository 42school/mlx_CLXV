
#include	"mlx_internal.h"
#include	"mlx__appkit_internal.h"

/*
** mlx_event enum (mlx_internal.h) -> our own backend event numbers.
** Same table shape as the Wayland/XCB backends: only mlx_key_hook /
** mlx_mouse_hook / mlx_expose_hook go through this conversion, raw
** mlx_hook() calls pass the backend-native number straight through.
*/
static int	mlx__appkit_convert_event[4] =
  {
    0,
    MLX_AK_EVENT_KEY_RELEASE,    /* MLX_EVENT_KEY - fired on release, like XCB */
    MLX_AK_EVENT_BUTTON_PRESS,   /* MLX_EVENT_MOUSE */
    MLX_AK_EVENT_EXPOSE          /* MLX_EVENT_EXPOSE */
  };

int	mlx__appkit_event_hook(mlx_backend_hooks_param_t *param)
{
  mlx__appkit_win_t	*win;

  win = (mlx__appkit_win_t *)(param->backend_win);
  if (param->mlx_event > 0)
    param->backend_event_number = mlx__appkit_convert_event[param->mlx_event];
  if (param->backend_event_number < 0 ||
      param->backend_event_number >= MLX_APPKIT_MAX_EVENT)
    return (-1);
  win->hook[param->backend_event_number] = param->backend_event_hook;
  win->hook_param[param->backend_event_number] = param->backend_event_param;
  return (0);
}
