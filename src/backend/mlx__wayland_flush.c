
#include	<stdlib.h>

#include	"mlx_internal.h"

#include	<wayland-client.h>

#include	"mlx__wayland_internal.h"


int	mlx__wayland_flush(mlx_backend_hooks_param_t *param)
{
  mlx__wayland_t	*wl;

  wl = (mlx__wayland_t *)(param->backend);
  return (wl_display_flush(wl->display) < 0 ? -1 : 0);
}
