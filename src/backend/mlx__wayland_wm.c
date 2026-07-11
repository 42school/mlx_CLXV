
/* mlx__wayland_wm.c - fake window manager: draws a minimal title bar
   and lets the user move/close the window, for compositors that grant
   no server-side decoration (see mlx__wayland_window.c and the
   xdg-decoration wiring in mlx__wayland_init.c). Entirely self
   contained: the rest of the Wayland backend only needs to create or
   destroy this title bar and forward pointer events to it. */

#define	_GNU_SOURCE

#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/mman.h>
#include	<linux/input-event-codes.h>

#include	"mlx_internal.h"
#include	"mlx_font.h"

#include	<wayland-client.h>

#include	"mlx__wayland_internal.h"


static mlx__wayland_win_t	*mlx__wayland_wm_win_from_titlebar(mlx__wayland_t *wl,
								    struct wl_surface *surface)
{
  mlx__wayland_win_t	*win;

  win = wl->windows;
  while (win && win->titlebar_surface != surface)
    win = win->next;
  return (win);
}

static int	mlx__wayland_wm_close_hit(mlx__wayland_win_t *win, int x, int y)
{
  return (x >= (int)(win->width - MLX_WM_TITLEBAR_CLOSE_WIDTH) &&
	  x < (int)win->width &&
	  y >= 0 && y < MLX_WM_TITLEBAR_HEIGHT);
}


#define	MLX_WM_TITLEBAR_RADIUS	6

/* true for pixels that fall outside the rounded top-left/top-right
   corners (the bottom stays square, it meets the window content) */
static int	mlx__wayland_wm_outside_corner(int x, int y, int width, int radius)
{
  int	cx;
  int	dx;
  int	dy;

  if (y >= radius)
    return (0);
  if (x < radius)
    cx = radius;
  else if (x >= width - radius)
    cx = width - radius - 1;
  else
    return (0);
  dx = x - cx;
  dy = y - radius;
  return (dx * dx + dy * dy > radius * radius);
}

/* true for pixels on either diagonal of an X mark, inset from the
   edges of a size x size square by 'margin' */
static int	mlx__wayland_wm_on_cross(int x, int y, int size, int margin)
{
  int	nx;
  int	ny;
  int	span;

  if (x < margin || x >= size - margin || y < margin || y >= size - margin)
    return (0);
  nx = x - margin;
  ny = y - margin;
  span = size - 2 * margin - 1;
  return (abs(nx - ny) <= 1 || abs(nx + ny - span) <= 1);
}

/* reuses the same font atlas as mlx_string_put() (mlx_font.c): a strip
   of 95 printable-ASCII glyphs, stored as a white-on-transparent mask
   tinted here with the bar's own text colour instead of the caller's */
#define	MLX_WM_FONT_NB_CHAR	95
#define	MLX_WM_FONT_GLYPH_W	((font_atlas.width / MLX_WM_FONT_NB_CHAR) - 2)

static void	mlx__wayland_wm_draw_char(uint32_t *pixels, unsigned int width,
					  unsigned int height, int dst_x,
					  int dst_y, char c)
{
  int		val;
  int		atlas_x0;
  int		x;
  int		y;
  int		px;
  int		py;
  unsigned char	alpha;
  uint32_t	bg;
  unsigned char	bg_r;
  unsigned char	bg_g;
  unsigned char	bg_b;

  val = (c >= 32 && c <= 126) ? c - 32 : 31;   /* 31 = '?', out-of-range fallback */
  atlas_x0 = val * (MLX_WM_FONT_GLYPH_W + 2);
  y = 0;
  while (y < (int)font_atlas.height)
    {
      x = 0;
      while (x < MLX_WM_FONT_GLYPH_W)
	{
	  px = dst_x + x;
	  py = dst_y + y;
	  if (px >= 0 && px < (int)width && py >= 0 && py < (int)height)
	    {
	      alpha = font_atlas.pixel_data[y * font_atlas.width *
					     font_atlas.bytes_per_pixel +
					     (atlas_x0 + x) * 4 + 3];
	      if (alpha)
		{
		  /* proper alpha blend against the bar colour already there,
		     same "over" compositing the GPU shader does for
		     mlx_string_put() (outColor = color * texture) instead
		     of a hard on/off threshold, which thickened every
		     stroke by fully painting even faint edge pixels */
		  bg = pixels[py * width + px];
		  bg_r = (bg >> 16) & 0xff;
		  bg_g = (bg >> 8) & 0xff;
		  bg_b = bg & 0xff;
		  bg_r = (0xe8 * alpha + bg_r * (255 - alpha)) / 255;
		  bg_g = (0xe8 * alpha + bg_g * (255 - alpha)) / 255;
		  bg_b = (0xe8 * alpha + bg_b * (255 - alpha)) / 255;
		  pixels[py * width + px] = 0xff000000 |
		    (bg_r << 16) | (bg_g << 8) | bg_b;
		}
	    }
	  x ++;
	}
      y ++;
    }
}

static void	mlx__wayland_wm_draw_title(uint32_t *pixels, unsigned int width,
					   unsigned int height, const char *title)
{
  int	x;
  int	y;
  int	max_x;

  if (title == NULL)
    return ;
  x = 8;
  y = ((int)height - (int)font_atlas.height) / 2;
  max_x = (int)width - MLX_WM_TITLEBAR_CLOSE_WIDTH - MLX_WM_FONT_GLYPH_W;
  while (*title && x < max_x)
    {
      /* glyph 0 (mapped from ' ') isn't actually blank in this atlas -
	 skip sampling it entirely rather than render whatever it is */
      if (*title != ' ')
	mlx__wayland_wm_draw_char(pixels, width, height, x, y, *title);
      x += MLX_WM_FONT_GLYPH_W;
      title ++;
    }
}

static struct wl_buffer	*mlx__wayland_wm_titlebar_buffer(mlx__wayland_t *wl,
								 unsigned int width,
								 unsigned int height,
								 const char *title)
{
  int			fd;
  struct wl_shm_pool	*pool;
  struct wl_buffer	*buffer;
  uint32_t		*pixels;
  size_t		size;
  int			x;
  int			y;
  int			close_x;

  size = (size_t)width * height * 4;
  fd = memfd_create("mlx-wl-titlebar", 0);
  if (fd < 0 || ftruncate(fd, size) < 0)
    {
      if (fd >= 0)
	close(fd);
      return (NULL);
    }
  pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (pixels == MAP_FAILED)
    {
      close(fd);
      return (NULL);
    }
  close_x = (int)width - MLX_WM_TITLEBAR_CLOSE_WIDTH;
  y = 0;
  while (y < (int)height)
    {
      x = 0;
      while (x < (int)width)
	{
	  if (mlx__wayland_wm_outside_corner(x, y, (int)width, MLX_WM_TITLEBAR_RADIUS))
	    pixels[y * (int)width + x] = 0x00000000;   /* corner cut, transparent */
	  else if (x >= close_x &&
		   mlx__wayland_wm_on_cross(x - close_x, y,
					     MLX_WM_TITLEBAR_CLOSE_WIDTH, 5))
	    pixels[y * (int)width + x] = 0xffe0e0e0;   /* close button, X mark */
	  else if (x >= close_x)
	    pixels[y * (int)width + x] = 0xff802020;   /* close button, dark red */
	  else
	    pixels[y * (int)width + x] = 0xff383838;   /* bar, dark grey */
	  x ++;
	}
      y ++;
    }
  mlx__wayland_wm_draw_title(pixels, width, height, title);
  munmap(pixels, size);
  pool = wl_shm_create_pool(wl->shm, fd, (int32_t)size);
  close(fd);
  buffer = wl_shm_pool_create_buffer(pool, 0, width, height, width * 4,
				      WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);
  return (buffer);
}


void	mlx__wayland_wm_titlebar_create(mlx__wayland_win_t *win)
{
  mlx__wayland_t	*wl;

  wl = win->wl;
  if (win->titlebar_surface || wl->subcompositor == NULL || wl->shm == NULL)
    return ;
  win->titlebar_surface = wl_compositor_create_surface(wl->compositor);
  if (win->titlebar_surface == NULL)
    return ;
  win->titlebar_subsurface =
    wl_subcompositor_get_subsurface(wl->subcompositor, win->titlebar_surface,
				     win->surface);
  if (win->titlebar_subsurface == NULL)
    {
      wl_surface_destroy(win->titlebar_surface);
      win->titlebar_surface = NULL;
      return ;
    }
  /* above the content, moves rigidly with it - no position syncing needed */
  wl_subsurface_set_position(win->titlebar_subsurface, 0,
			      -MLX_WM_TITLEBAR_HEIGHT);
  /* let the compositor (task switcher, click-to-raise...) treat the
     bar + content as a single window rather than just the content */
  xdg_surface_set_window_geometry(win->xdg_surface, 0, -MLX_WM_TITLEBAR_HEIGHT,
				   win->width,
				   win->height + MLX_WM_TITLEBAR_HEIGHT);

  win->titlebar_buffer = mlx__wayland_wm_titlebar_buffer(wl, win->width,
							  MLX_WM_TITLEBAR_HEIGHT,
							  win->title);
  if (win->titlebar_buffer == NULL)
    return ;
  wl_surface_attach(win->titlebar_surface, win->titlebar_buffer, 0, 0);
  wl_surface_damage(win->titlebar_surface, 0, 0, win->width,
		     MLX_WM_TITLEBAR_HEIGHT);
  wl_surface_commit(win->titlebar_surface);
  /* a synchronized sub-surface only becomes visible once its parent
     commits; if the main surface was already mapped (reactive path,
     triggered from a late decoration 'configure'), nudge it now */
  if (win->configured)
    wl_surface_commit(win->surface);
}

void	mlx__wayland_wm_titlebar_destroy(mlx__wayland_win_t *win)
{
  if (win->titlebar_buffer)
    wl_buffer_destroy(win->titlebar_buffer);
  if (win->titlebar_subsurface)
    wl_subsurface_destroy(win->titlebar_subsurface);
  if (win->titlebar_surface)
    wl_surface_destroy(win->titlebar_surface);
}


int	mlx__wayland_wm_pointer_enter(mlx__wayland_t *wl, struct wl_surface *surface)
{
  return (mlx__wayland_wm_win_from_titlebar(wl, surface) != NULL);
}

int	mlx__wayland_wm_pointer_leave(mlx__wayland_t *wl, struct wl_surface *surface)
{
  return (mlx__wayland_wm_win_from_titlebar(wl, surface) != NULL);
}

int	mlx__wayland_wm_pointer_motion(mlx__wayland_t *wl, struct wl_surface *surface,
			       int x, int y)
{
  mlx__wayland_win_t	*win;

  win = mlx__wayland_wm_win_from_titlebar(wl, surface);
  if (win == NULL)
    return (0);
  win->titlebar_ptr_x = x;
  win->titlebar_ptr_y = y;
  return (1);
}

int	mlx__wayland_wm_pointer_button(mlx__wayland_t *wl, struct wl_surface *surface,
			       uint32_t serial, uint32_t button, uint32_t state)
{
  mlx__wayland_win_t	*win;

  win = mlx__wayland_wm_win_from_titlebar(wl, surface);
  if (win == NULL)
    return (0);
  if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
    {
      if (mlx__wayland_wm_close_hit(win, win->titlebar_ptr_x, win->titlebar_ptr_y))
	mlx__wayland_queue_push(wl, win, MLX_WL_EVENT_CLOSE, 0, 0, 0);
      else
	xdg_toplevel_move(win->xdg_toplevel, wl->seat, serial);
    }
  return (1);
}
