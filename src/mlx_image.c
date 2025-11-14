
#include	<stdlib.h>
#include	<stdint.h>
#include	<stdio.h>
#include	<strings.h>

#include	"mlx_internal.h"


void	*mlx_new_image(mlx_t *mlx, unsigned int size_x, unsigned int size_y)
{
  mlx_img_t			*img;
  mlx_gpu_hooks_param_t		gpup;

  if ( (img = malloc(sizeof(*img))) == NULL )
      return (NULL);
  img->mlx = mlx;
  img->gpu_img = NULL;

  bzero(&gpup, sizeof(gpup));
  gpup.gpu = mlx->gpu;
  gpup.dst.width = size_x;
  gpup.dst.height = size_y;
  gpup.backend = NULL;
  gpup.backend_win = NULL;
  gpup.data = NULL;
  gpup.format = 0;
  img->gpu_img = MLX_EXISTS_P(mlx_gpu_hooks[mlx->gpu_type].image, &gpup);

  if (!img->gpu_img)
    {
      free(img);
      return (NULL);
    }

  img->data = gpup.data;
  img->format = gpup.format;
  img->sizeline = gpup.sizeline;
  return (img);
}


void	mlx_destroy_image(mlx_t *mlx, mlx_img_t *img)
{
  mlx_gpu_hooks_param_t		gpup;

  bzero(&gpup, sizeof(gpup));
  gpup.gpu = mlx->gpu;
  gpup.gpu_img = img->gpu_img;
  MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].image_release, &gpup);
  free(img);
}


int	mlx_put_image_to_window(mlx_t *mlx, mlx_win_t *win, mlx_img_t *img,
				int x, int y)
{
  mlx_gpu_hooks_param_t         gpup;

  bzero(&gpup, sizeof(gpup));
  gpup.gpu = mlx->gpu;
  gpup.gpu_img = img->gpu_img;
  gpup.gpu_win = win->gpu_win;
  gpup.dst.x = x;
  gpup.dst.y = y;
  gpup.dst.width = 0; // means regular put image
  gpup.dst.height = 0;
  MLX_EXISTS_I(mlx_gpu_hooks[mlx->gpu_type].image_put, &gpup);
  return (0);
}

unsigned char	*mlx_get_data_addr(mlx_img_t *img, unsigned int *bpp,
				   unsigned int *sizeline,
				   unsigned int *format)
{
  *bpp = 32; // always in our modern architecture
  *format = img->format;
  *sizeline = img->sizeline;
  return (img->data);
}

