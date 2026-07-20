
#include	"mlx_internal.h"
#include	"mlx__appkit_internal.h"

int	mlx__appkit_flush(mlx_backend_hooks_param_t *param)
{
  (void)param;
  /* no client-server round trip in Cocoa, nothing to flush */
  return (0);
}
