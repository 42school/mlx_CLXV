
#include	<stdio.h>

#include	"mlx_internal.h"

#include	<wayland-client.h>

#include	"mlx__wayland_internal.h"


static int	mlx__wayland_extra_set_mouse(mlx__wayland_t *wl,
					     mlx__wayland_win_t *win,
					     int x, int y)
{
  static int	warned = 0;

#ifdef MLX_WAYLAND_HAVE_POINTER_WARP
  /* pointer-warp-v1 is a staging protocol (not part of core Wayland
     yet): available on recent Mutter/KWin, absent elsewhere - this is
     exactly what Xwayland itself uses to emulate XWarpPointer */
  if (wl->pointer_warp && wl->pointer)
    {
      wp_pointer_warp_v1_warp_pointer(wl->pointer_warp, win->surface,
				       wl->pointer, wl_fixed_from_int(x),
				       wl_fixed_from_int(y),
				       wl->pointer_enter_serial);
      return (0);
    }
#else
  (void)win; (void)x; (void)y;
#endif
  /* no pointer-warp support: core Wayland has no other way for a
     client to move the cursor, only the compositor (or the user) can */
  (void)wl;
  if (warned == 0)
    {
      fprintf(stderr, "Mlx - Wayland: mlx_mouse_move() is not supported "
	      "(compositor lacks the pointer-warp-v1 protocol)\n");
      warned = 1;
    }
  return (-1);
}


static int	mlx__wayland_extra_get_mouse(mlx__wayland_win_t *win,
					     int *x, int *y)
{
  *x = win->ptr_x;
  *y = win->ptr_y;
  return (0);
}


int	mlx__wayland_extra(mlx_backend_hooks_param_t *param)
{
  mlx__wayland_t	*wl;
  mlx__wayland_win_t	*win;

  wl = (mlx__wayland_t *)param->backend;
  win = (mlx__wayland_win_t *)param->backend_win;

  if (param->action == MLX_BE_MOUSE_HIDE)
    {
      mlx__wayland_cursor_set(wl, 0);
      return (0);
    }
  if (param->action == MLX_BE_MOUSE_SHOW)
    {
      mlx__wayland_cursor_set(wl, 1);
      return (0);
    }
  if (param->action == MLX_BE_MOUSE_GET)
    return (mlx__wayland_extra_get_mouse(win, &(param->size.x), &(param->size.y)));
  if (param->action == MLX_BE_MOUSE_SET)
    return (mlx__wayland_extra_set_mouse(wl, win, param->size.x, param->size.y));
  if (param->action == MLX_BE_KEY_AUTO_OFF || param->action == MLX_BE_KEY_AUTO_ON)
    /* wl_keyboard never auto-repeats key presses itself (that's left to
       the client to synthesize, which mlx does not do), and mlx_key_hook
       only fires on release anyway, so there is nothing to toggle here */
    return (0);
  return (-1);
}
