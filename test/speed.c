
#include	<stdlib.h>
#include	<sys/time.h>
#include	<stdio.h>
#include	"mlx.h"

void *mlx;
void *win;
void *img;
unsigned char *data;
int gl_count = 0;
int last_tv;

int	do_loop(void *dummy)
{
  int	i;
  struct timeval tv;
  static int toggle = 0;

  i = 0;
  while (i < 800*800)
    {
      if (toggle)
	*((unsigned int *)(data+4*i)) = 0xFFFF0000;
      else
	*((unsigned int *)(data+4*i)) = 0xFF0000FF;
      i ++;
    }
  toggle = 1 - toggle;
  mlx_put_image_to_window(mlx, win, img, 0, 0);
  gl_count ++;
  gettimeofday(&tv, NULL);
  if (tv.tv_sec > last_tv+5)
    {
      printf("fps: %.2f\n", gl_count/5.0);
      gl_count = 0;
      last_tv = tv.tv_sec;
    }
  return (0);
}



int	main()
{
  unsigned int bpp, sl, format;
  struct timeval tv;
  
  mlx = mlx_init();
  win = mlx_new_window(mlx, 800, 800, "speed test");
  img = mlx_new_image(mlx, 800, 800);
  data = mlx_get_data_addr(img, &bpp, &sl, &format);
  mlx_loop_hook(mlx, do_loop, (void *)0);
  gettimeofday(&tv, NULL);
  last_tv = tv.tv_sec;
  mlx_loop(mlx);
}
