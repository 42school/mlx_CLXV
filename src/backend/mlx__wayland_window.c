
#include	<stdlib.h>
#include	<string.h>
#include	<strings.h>

#include	"mlx_internal.h"

#include	<wayland-client.h>

#include	"mlx__wayland_internal.h"


static void	*mlx__wayland_window_error(mlx__wayland_t *wl,
					   mlx__wayland_win_t *win)
{
  if (wl && win)
    {
      mlx__wayland_win_remove(wl, win);
      if (win->xdg_toplevel)
	xdg_toplevel_destroy(win->xdg_toplevel);
      if (win->xdg_surface)
	xdg_surface_destroy(win->xdg_surface);
      if (win->surface)
	wl_surface_destroy(win->surface);
      free(win);
    }
  return (NULL);
}


void	mlx__wayland_window_destroy(mlx_backend_hooks_param_t *param)
{
  mlx__wayland_t	*wl;
  mlx__wayland_win_t	*win;

  wl = (mlx__wayland_t *)(param->backend);
  win = (mlx__wayland_win_t *)(param->backend_win);
  mlx__wayland_window_error(wl, win);
}


static void	mlx__wayland_xdg_surface_configure(void *data,
						   struct xdg_surface *xdg_surface,
						   uint32_t serial)
{
  mlx__wayland_win_t	*win;

  win = (mlx__wayland_win_t *)data;
  xdg_surface_ack_configure(xdg_surface, serial);
  wl_surface_commit(win->surface);
  if (win->configured == 0)
    {
      /* Wayland has no Expose event: the compositor keeps the buffer
	 alive itself, so we only fire this once, right after the very
	 first configure, to mimic what X servers do in practice today */
      win->configured = 1;
      mlx__wayland_queue_push(win->wl, win, MLX_WL_EVENT_EXPOSE, 0, 0, 0);
    }
}

static const struct xdg_surface_listener	mlx__wayland_xdg_surface_listener =
  {
    .configure = mlx__wayland_xdg_surface_configure
  };


static void	mlx__wayland_xdg_toplevel_configure(void *data,
						    struct xdg_toplevel *tl,
						    int32_t width,
						    int32_t height,
						    struct wl_array *states)
{
  (void)data; (void)tl; (void)width; (void)height; (void)states;
  /* mlx windows are fixed-size (anti-resize), nothing to resize here */
}

static void	mlx__wayland_xdg_toplevel_close(void *data,
						struct xdg_toplevel *tl)
{
  mlx__wayland_win_t	*win;

  (void)tl;
  win = (mlx__wayland_win_t *)data;
  mlx__wayland_queue_push(win->wl, win, MLX_WL_EVENT_CLOSE, 0, 0, 0);
}

static const struct xdg_toplevel_listener	mlx__wayland_xdg_toplevel_listener =
  {
    .configure = mlx__wayland_xdg_toplevel_configure,
    .close = mlx__wayland_xdg_toplevel_close
  };


void	*mlx__wayland_window(mlx_backend_hooks_param_t *param)
{
  mlx__wayland_t	*wl;
  mlx__wayland_win_t	*win;

  wl = (mlx__wayland_t *)(param->backend);
  if ((win = malloc(sizeof(*win))) == NULL)
    return (NULL);
  bzero(win, sizeof(*win));
  win->wl = wl;
  win->width = param->size.width;
  win->height = param->size.height;
  mlx__wayland_win_add(wl, win);

  win->surface = wl_compositor_create_surface(wl->compositor);
  if (win->surface == NULL)
    return (mlx__wayland_window_error(wl, win));

  win->xdg_surface = xdg_wm_base_get_xdg_surface(wl->xdg_wm_base,
						  win->surface);
  if (win->xdg_surface == NULL)
    return (mlx__wayland_window_error(wl, win));
  xdg_surface_add_listener(win->xdg_surface,
			    &mlx__wayland_xdg_surface_listener, win);

  win->xdg_toplevel = xdg_surface_get_toplevel(win->xdg_surface);
  if (win->xdg_toplevel == NULL)
    return (mlx__wayland_window_error(wl, win));
  xdg_toplevel_add_listener(win->xdg_toplevel,
			     &mlx__wayland_xdg_toplevel_listener, win);

  if (param->title)
    xdg_toplevel_set_title(win->xdg_toplevel, param->title);
  xdg_toplevel_set_app_id(win->xdg_toplevel, "minilibx");

  /* anti-resize: lock min and max size to the requested size,
     mirrors mlx__xcb_anti_resize_win() */
  xdg_toplevel_set_min_size(win->xdg_toplevel, win->width, win->height);
  xdg_toplevel_set_max_size(win->xdg_toplevel, win->width, win->height);

  /* trigger the first configure round-trip; no buffer is attached yet,
     the GPU backend attaches one once the swapchain is created */
  wl_surface_commit(win->surface);
  wl_display_roundtrip(wl->display);

  return (win);
}
