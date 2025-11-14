
#include	<stdlib.h>

#include        "mlx_internal.h"


int	mlx_loop_hook(mlx_t *mlx, int (*funct_ptr)(), void *param)
{
  mlx->loop_hook = funct_ptr;
  mlx->loop_hook_param = param;
  return (0);
}
