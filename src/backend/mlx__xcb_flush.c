
#include	<stdlib.h>

#include	"mlx_internal.h"

#include	<xcb/xcb.h>
#include	<xcb/xcb_keysyms.h>

#include	"mlx__xcb_internal.h"


int		mlx__xcb_flush(mlx_backend_hooks_param_t *param)
{
  mlx__xcb_t	*xcb;

  xcb = (mlx__xcb_t *)(param->backend);
  return (xcb_flush(xcb->cnx));
}
