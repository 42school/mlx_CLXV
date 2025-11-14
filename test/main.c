

#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<err.h>

#include	"mlx.h"

#ifndef NULL
#define NULL	((void *)0)
#endif

typedef struct	img_s
{
  void		*img;
  unsigned int	width;
  unsigned int	height;
  unsigned char	*data;
  unsigned int	sl;
  unsigned int	bpp;
  unsigned int	format;
} img_t;


typedef struct	xvar_s
{
  void *mlx;
  unsigned int screen_w;
  unsigned int screen_h;
  void *win_1;
  void *win_2;
  img_t img_1;
  img_t img_2;
  img_t img_png;
  img_t img_xpm;
  int	imgidx;
} xvar_t;


void	draw_colormap(xvar_t *xvar)
{
  int	i,j;
  unsigned int col;
  
  i = 0;
  while (i < 400)
    {
      j = 0;
      while (j < 400)
	{
	  col = 0xFF000000 | (((0xFF * i) / 400)<<16) | (((0xFF * j) / 400)<<8) | (0xFF * (400-(i+j)/2)) / 400;
	  mlx_pixel_put(xvar->mlx, xvar->win_1, i, j, col);
	  j ++;
	}
      i ++;
    }
}


int	gere_key_press(unsigned int key, void *param)
{
  printf("Pressed key %d\n", key);
  return (0);
}


int	gere_key(unsigned int key, void *param)
{
  int x, y;
  xvar_t *xvar;

  xvar = (xvar_t *)param;
  printf("Got key %d: ", key);
  if (key == 113) { mlx_do_key_autorepeatoff(xvar->mlx);
    printf("key repeat off\n"); return (0); }
  if (key == 119) { mlx_do_key_autorepeaton(xvar->mlx);
    printf("key repeat on\n"); return (0); }
  if (key == 101) { draw_colormap(xvar);
    printf("colormap\n"); return (0); }
  if (key == 114) { mlx_mouse_hide(xvar->mlx);
    printf("mouse hide\n"); return (0); }
  if (key == 116) { mlx_mouse_show(xvar->mlx);
    printf("mouse show\n"); return (0); }
  if (key == 121) { mlx_mouse_move(xvar->win_1, 200, 200);
    printf("mouse move\n"); return (0); }
  if (key == 117) { mlx_mouse_get_pos(xvar->win_1, &x, &y);
    printf("current mouse pos is %d x %d\n", x, y); return (0); }
  if (key == 105)
    {
      mlx_sync(xvar->mlx, MLX_SYNC_IMAGE_WRITABLE, xvar->img_2.img);
      memset(xvar->img_2.data, 0xFF, xvar->img_2.sl*100);
      mlx_put_image_to_window(xvar->mlx, xvar->win_1, xvar->img_2.img, 50, 50);
      x = 0;
      while (x < (int)xvar->img_2.sl*100)
	{
	  *(unsigned int *)(xvar->img_2.data + x) = 0xFFFF0000;
	  x += 4;
	}
      mlx_put_image_to_window(xvar->mlx, xvar->win_1, xvar->img_2.img, 250, 250);
      printf("update image without sync - most likely 2 red squares\n");
      return (0);
    }
  if (key == 111)
    {
      mlx_sync(xvar->mlx, MLX_SYNC_IMAGE_WRITABLE, xvar->img_2.img);
      memset(xvar->img_2.data, 0xFF, xvar->img_2.sl*100);
      mlx_put_image_to_window(xvar->mlx, xvar->win_1, xvar->img_2.img, 50, 50);
      mlx_sync(xvar->mlx, MLX_SYNC_IMAGE_WRITABLE, xvar->img_2.img);
      x = 0;
      while (x < (int)xvar->img_2.sl*100)
	{
	  *(unsigned int *)(xvar->img_2.data + x) = 0xFFFF0000;
	  x += 4;
	}
      mlx_put_image_to_window(xvar->mlx, xvar->win_1, xvar->img_2.img, 250, 250);
      printf("update image with sync - white and red squares\n");
      return (0);
    }
  
  /* default */
  printf("clear and string put\n");
  mlx_clear_window(xvar->mlx, xvar->win_1);
  mlx_string_put(xvar->mlx, xvar->win_1, 20, 20, 0xFFFF00FF, "Hello MLX!");
  return (0);
}

int	gere_expose(void *param)
{
  xvar_t *xvar;
  xvar = (xvar_t *)param;
  printf("Expose !\n");
  mlx_put_image_to_window(xvar->mlx, xvar->win_1, xvar->img_1.img, 0, 0);
  mlx_put_image_to_window(xvar->mlx, xvar->win_1, xvar->img_1.img, 201, 201);
  return (0);
}



int	gere_mouse(unsigned int button, unsigned int x, unsigned int y, xvar_t *xvar, void *win)
{
  printf("Got mouse : %d at %dx%d\n", button, x, y);
  if (button == 1)
    {
      mlx_put_image_to_window(xvar->mlx, win, xvar->img_1.img, 100, 100);
      return (0);
    }
  if (button == 3)
    {
      if ((xvar->imgidx++) % 2)
	mlx_put_image_to_window(xvar->mlx, win, xvar->img_png.img, x, y);
      else
	mlx_put_image_to_window(xvar->mlx, win, xvar->img_xpm.img, x, y);
    }
  return (0);
}

int	gere_mouse_1(unsigned int button, unsigned int x, unsigned int y, void *param)
{
  xvar_t *xvar;
  
  xvar = (xvar_t *)param;
  return (gere_mouse(button, x, y, xvar, xvar->win_1));
}

int	gere_mouse_2(unsigned int button, unsigned int x, unsigned int y, void *param)
{
  xvar_t *xvar;

  xvar = (xvar_t *)param;
  return (gere_mouse(button, x, y, xvar, xvar->win_2));
}


int	gere_close_1(void *param)
{
  xvar_t *xvar;

  xvar = (xvar_t *)param;
  mlx_loop_exit(xvar->mlx);
  return (0);
}

int	gere_close_2(void *param)
{
  xvar_t *xvar;

  xvar = (xvar_t *)param;
  mlx_destroy_window(xvar->mlx, xvar->win_2);
  xvar->win_2 = NULL;
  return (0);
}

int main()
{
  xvar_t	xvar;
  
  if ((xvar.mlx = mlx_init()) == NULL)
    errx(1, "Can't mlx_init");

  mlx_get_screen_size(xvar.mlx, &(xvar.screen_w), &(xvar.screen_h));
  printf("Screen size: %d x %d\n", xvar.screen_w, xvar.screen_h);

  if ((xvar.win_1 = mlx_new_window(xvar.mlx, 400, 400, "MLX main win")) == NULL)
    errx(1, "Can't create new window");

  if ((xvar.win_2 = mlx_new_window(xvar.mlx, 150, 150, "Secondary window")) == NULL)
    errx(1, "Can't create new window");

  if ((xvar.img_1.img = mlx_new_image(xvar.mlx, 200, 200)) == NULL)
    errx(1, "Can't create new image 1");
  xvar.img_1.width = 200;
  xvar.img_1.height = 200;
  xvar.img_1.data = mlx_get_data_addr(xvar.img_1.img, &(xvar.img_1.bpp),
				      &(xvar.img_1.sl), &(xvar.img_1.format));
  
  memset(xvar.img_1.data, 0x80, xvar.img_1.sl*200);
  memset(xvar.img_1.data, 0xFF, xvar.img_1.sl*100);
  *((unsigned int *)(xvar.img_1.data) + 0*200) = 0xFFFF0000;
  *((unsigned int *)(xvar.img_1.data) + 1*200+1) = 0xFFFF0000;
  *((unsigned int *)(xvar.img_1.data) + 199*200+199) = 0xFFFF0000;
  *((unsigned int *)(xvar.img_1.data) + 198*200+198) = 0xFFFF0000;

  if ((xvar.img_2.img = mlx_new_image(xvar.mlx, 100, 100)) == NULL)
      errx(1, "Can't create new image 2");
  xvar.img_2.width = 100;
  xvar.img_2.height = 100;
  xvar.img_2.data = mlx_get_data_addr(xvar.img_2.img, &(xvar.img_2.bpp),
				      &(xvar.img_2.sl), &(xvar.img_2.format));

  if ((xvar.img_png.img = mlx_png_file_to_image(xvar.mlx, "puffy_small.png",
						&(xvar.img_png.width),
						&(xvar.img_png.height))) == NULL)
    errx(1, "Can't load png");
  xvar.img_png.data = mlx_get_data_addr(xvar.img_png.img, &(xvar.img_png.bpp),
				&(xvar.img_png.sl), &(xvar.img_png.format));

  if ((xvar.img_xpm.img = mlx_xpm_file_to_image(xvar.mlx, "Dont_panic.xpm",
						&(xvar.img_xpm.width),
						&(xvar.img_xpm.height))) == NULL)
    errx(1, "Can't load xpm");
  xvar.img_xpm.data = mlx_get_data_addr(xvar.img_xpm.img, &(xvar.img_xpm.bpp),
				&(xvar.img_xpm.sl), &(xvar.img_xpm.format));
  
  mlx_key_hook(xvar.win_1, gere_key, &xvar);
  mlx_hook(xvar.win_2, 2, 1, (int (*)(void *))gere_key_press, &xvar);
  mlx_expose_hook(xvar.win_1, gere_expose, &xvar);
  mlx_mouse_hook(xvar.win_1, gere_mouse_1, &xvar);
  mlx_mouse_hook(xvar.win_2, gere_mouse_2, &xvar);
  mlx_hook(xvar.win_1, 33, 0, gere_close_1, &xvar);
  mlx_hook(xvar.win_2, 33, 0, gere_close_2, &xvar);

  printf("On main window:\n");
  printf(" mouse button 1: place white/gray image in 0x0 and 200x200\n");
  printf(" mouse button 2: place png image and xpm image, alternatively\n");
  printf(" try keys QWERTYUIO and others\n");
  printf(" click window's X button to end the program\n");
  printf("On secondary window (smaller):\n");
  printf(" show key pressed for auto repeat\n");
  printf(" click window's X button to close it\n");
  mlx_loop(xvar.mlx);
  
  printf("destroy xpm\n");
  mlx_destroy_image(xvar.mlx, xvar.img_xpm.img);
  printf("destroy png\n");
  mlx_destroy_image(xvar.mlx, xvar.img_png.img);
  printf("destroy imgs\n");
  mlx_destroy_image(xvar.mlx, xvar.img_1.img);
  mlx_destroy_image(xvar.mlx, xvar.img_2.img);
  printf("destroy win(s)\n");
  mlx_destroy_window(xvar.mlx, xvar.win_1);
  if (xvar.win_2)
    mlx_destroy_window(xvar.mlx, xvar.win_2);
  printf("destroy mlx\n");
  mlx_release(xvar.mlx);
}
