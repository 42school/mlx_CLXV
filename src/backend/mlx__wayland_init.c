
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<strings.h>

#include	"mlx_internal.h"

#include	<wayland-client.h>

#include	"mlx__wayland_internal.h"


static void	*mlx__wayland_error(mlx__wayland_t *wl)
{
  if (wl)
    {
      mlx__wayland_cursor_destroy(wl);
      mlx__wayland_seat_destroy(wl);
#ifdef MLX_WAYLAND_HAVE_POINTER_WARP
      if (wl->pointer_warp)
	wp_pointer_warp_v1_destroy(wl->pointer_warp);
#endif
      if (wl->xdg_wm_base)
	xdg_wm_base_destroy(wl->xdg_wm_base);
      if (wl->output)
	wl_output_destroy(wl->output);
      if (wl->shm)
	wl_shm_destroy(wl->shm);
      if (wl->compositor)
	wl_compositor_destroy(wl->compositor);
      if (wl->registry)
	wl_registry_destroy(wl->registry);
      if (wl->display)
	wl_display_disconnect(wl->display);
      free(wl);
    }
  return (NULL);
}


void	mlx__wayland_destroy(mlx_backend_hooks_param_t *param)
{
  mlx__wayland_t	*wl;

  wl = (mlx__wayland_t *)(param->backend);
  mlx__wayland_error(wl);
}


static void	mlx__wayland_xdg_wm_base_ping(void *data,
					      struct xdg_wm_base *xdg_wm_base,
					      uint32_t serial)
{
  (void)data;
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener	mlx__wayland_xdg_wm_base_listener =
  {
    .ping = mlx__wayland_xdg_wm_base_ping
  };


static void	mlx__wayland_output_geometry(void *data, struct wl_output *o,
					     int32_t x, int32_t y,
					     int32_t phys_w, int32_t phys_h,
					     int32_t subpixel,
					     const char *make, const char *model,
					     int32_t transform)
{
  (void)data; (void)o; (void)x; (void)y; (void)phys_w; (void)phys_h;
  (void)subpixel; (void)make; (void)model; (void)transform;
}

static void	mlx__wayland_output_mode(void *data, struct wl_output *o,
					 uint32_t flags, int32_t width,
					 int32_t height, int32_t refresh)
{
  mlx__wayland_t	*wl;

  (void)o; (void)refresh;
  wl = (mlx__wayland_t *)data;
  if (flags & WL_OUTPUT_MODE_CURRENT)
    {
      wl->screen_width = width;
      wl->screen_height = height;
    }
}

static void	mlx__wayland_output_done(void *data, struct wl_output *o)
{
  (void)data; (void)o;
}

static void	mlx__wayland_output_scale(void *data, struct wl_output *o,
					  int32_t factor)
{
  (void)data; (void)o; (void)factor;
}

/* libwayland-client requires every opcode valid for the bound protocol
   version to have a non-NULL listener entry - it does NOT silently
   skip unset ones, it logs "listener function for opcode N is NULL"
   and aborts. geometry/done/scale are all valid since wl_output v1/v2
   (we bind v2), so they must be here even though only mode is used. */
static const struct wl_output_listener	mlx__wayland_output_listener =
  {
    .geometry = mlx__wayland_output_geometry,
    .mode = mlx__wayland_output_mode,
    .done = mlx__wayland_output_done,
    .scale = mlx__wayland_output_scale
  };


static void	mlx__wayland_registry_global(void *data,
					     struct wl_registry *registry,
					     uint32_t name,
					     const char *interface,
					     uint32_t version)
{
  mlx__wayland_t	*wl;

  wl = (mlx__wayland_t *)data;
  if (strcmp(interface, wl_compositor_interface.name) == 0)
    wl->compositor = wl_registry_bind(registry, name,
				       &wl_compositor_interface, 4);
  else if (strcmp(interface, wl_shm_interface.name) == 0)
    wl->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
  else if (strcmp(interface, wl_seat_interface.name) == 0)
    /* stick to version 1: only 'capabilities' is needed, and it keeps
       the pointer/keyboard listeners below to their simplest, stable
       set of events (no frame/axis_source/repeat_info bookkeeping) */
    wl->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
  else if (strcmp(interface, wl_output_interface.name) == 0)
    {
      wl->output = wl_registry_bind(registry, name, &wl_output_interface, 2);
      wl_output_add_listener(wl->output, &mlx__wayland_output_listener, wl);
    }
  else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
      wl->xdg_wm_base = wl_registry_bind(registry, name,
					  &xdg_wm_base_interface, 1);
      xdg_wm_base_add_listener(wl->xdg_wm_base,
				&mlx__wayland_xdg_wm_base_listener, wl);
    }
#ifdef MLX_WAYLAND_HAVE_POINTER_WARP
  else if (strcmp(interface, wp_pointer_warp_v1_interface.name) == 0)
    /* optional: only present on compositors implementing this
       still-in-testing protocol (e.g. recent Mutter/KWin); absence
       just means mlx_mouse_move() stays unsupported on this backend */
    wl->pointer_warp = wl_registry_bind(registry, name,
					 &wp_pointer_warp_v1_interface, 1);
#endif
}

static void	mlx__wayland_registry_global_remove(void *data,
						    struct wl_registry *registry,
						    uint32_t name)
{
  (void)data; (void)registry; (void)name;
}

static const struct wl_registry_listener	mlx__wayland_registry_listener =
  {
    .global = mlx__wayland_registry_global,
    .global_remove = mlx__wayland_registry_global_remove
  };


void	*mlx__wayland_init(mlx_backend_hooks_param_t *param)
{
  mlx__wayland_t	*wl;

  if ((wl = malloc(sizeof(*wl))) == NULL)
    return (NULL);
  bzero(wl, sizeof(*wl));

  wl->display = wl_display_connect(NULL);
  if (wl->display == NULL)
    return (mlx__wayland_error(wl));

  wl->registry = wl_display_get_registry(wl->display);
  wl_registry_add_listener(wl->registry, &mlx__wayland_registry_listener, wl);
  wl_display_roundtrip(wl->display);   /* fetch the globals list */
  wl_display_roundtrip(wl->display);   /* let bound globals fire their own events (seat capabilities, output mode ...) */

  if (wl->compositor == NULL || wl->shm == NULL || wl->xdg_wm_base == NULL)
    return (mlx__wayland_error(wl));

  wl->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (wl->xkb_context == NULL)
    return (mlx__wayland_error(wl));

  /* cursor state must exist before the seat is wired up: a pointer
     enter event, delivered as soon as the seat listener is attached,
     would otherwise reach mlx__wayland_cursor_set() with no surface */
  if (mlx__wayland_cursor_init(wl))
    return (mlx__wayland_error(wl));

  if (wl->seat && mlx__wayland_seat_init(wl))
    return (mlx__wayland_error(wl));

  if (wl->screen_width == 0 || wl->screen_height == 0)
    {
      /* no wl_output reported a mode (headless / nested compositor):
	 fall back to a sane default, screen size is only informative */
      wl->screen_width = 1920;
      wl->screen_height = 1080;
    }
  param->size.width = wl->screen_width;
  param->size.height = wl->screen_height;

  return (wl);
}
