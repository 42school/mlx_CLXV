
#include	<stdlib.h>
#include	<stdio.h>

#include	"mlx_internal.h"

#include	<wayland-client.h>
#include	<wayland-cursor.h>

#include	"mlx__wayland_internal.h"


int	mlx__wayland_cursor_init(mlx__wayland_t *wl)
{
  const char	*theme_name;
  int		size;

  wl->cursor_surface = wl_compositor_create_surface(wl->compositor);
  if (wl->cursor_surface == NULL)
    return (1);

  theme_name = getenv("XCURSOR_THEME");
  size = getenv("XCURSOR_SIZE") ? atoi(getenv("XCURSOR_SIZE")) : 24;
  wl->cursor_theme = wl_cursor_theme_load(theme_name, size, wl->shm);
  if (wl->cursor_theme)
    wl->cursor_default = wl_cursor_theme_get_cursor(wl->cursor_theme, "left_ptr");
  if (wl->cursor_default == NULL)
    fprintf(stderr, "Mlx - Wayland: no 'left_ptr' cursor found in the cursor theme\n");

  wl->cursor_visible = 1;
  return (0);
}


void	mlx__wayland_cursor_destroy(mlx__wayland_t *wl)
{
  if (wl->cursor_theme)
    wl_cursor_theme_destroy(wl->cursor_theme);
  if (wl->cursor_surface)
    wl_surface_destroy(wl->cursor_surface);
}


void	mlx__wayland_cursor_set(mlx__wayland_t *wl, int visible)
{
  struct wl_cursor_image	*image;

  wl->cursor_visible = visible;
  if (wl->pointer == NULL || wl->pointer_focus == NULL)
    return ;
  if (visible == 0)
    {
      /* a NULL surface hides the pointer image, per protocol */
      wl_pointer_set_cursor(wl->pointer, wl->pointer_enter_serial, NULL, 0, 0);
      return ;
    }
  if (wl->cursor_default == NULL)
    return ;   /* no theme available, leave whatever cursor is currently set */
  image = wl->cursor_default->images[0];
  wl_surface_attach(wl->cursor_surface, wl_cursor_image_get_buffer(image), 0, 0);
  wl_surface_damage(wl->cursor_surface, 0, 0, image->width, image->height);
  wl_surface_commit(wl->cursor_surface);
  wl_pointer_set_cursor(wl->pointer, wl->pointer_enter_serial, wl->cursor_surface,
			image->hotspot_x, image->hotspot_y);
}
