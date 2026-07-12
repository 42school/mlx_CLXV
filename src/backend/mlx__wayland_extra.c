
#include	<stdio.h>

#include	"mlx_internal.h"

#include	<wayland-client.h>

#include	"mlx__wayland_internal.h"


#ifdef MLX_WAYLAND_HAVE_POINTER_CONSTRAINTS
/* only used to learn whether the lock actually activated before the
   blocking round-trip below returns; mlx_mouse_move() is a synchronous
   call, but lock_pointer() is inherently async (the compositor may
   need e.g. the surface to already have pointer focus) */
static void	mlx__wayland_lock_locked(void *data,
					 struct zwp_locked_pointer_v1 *lock)
{
  (void)lock;
  *(int *)data = 1;
}

static void	mlx__wayland_lock_unlocked(void *data,
					   struct zwp_locked_pointer_v1 *lock)
{
  (void)data; (void)lock;
}

static const struct zwp_locked_pointer_v1_listener	mlx__wayland_lock_listener =
  {
    .locked = mlx__wayland_lock_locked,
    .unlocked = mlx__wayland_lock_unlocked
  };

/* Xwayland's own trick for emulating XWarpPointer: lock the pointer,
   set a cursor position hint, then unlock - the compositor "may warp
   the cursor position to the set cursor position hint" on unlock. Far
   more broadly supported than pointer-warp-v1 (this protocol has been
   around since 2014/2015, used by any app doing FPS-style mouselook). */
static int	mlx__wayland_extra_set_mouse_constraints(mlx__wayland_t *wl,
							  mlx__wayland_win_t *win,
							  int x, int y)
{
  struct zwp_locked_pointer_v1	*lock;
  int				activated;

  if (wl->pointer_constraints == NULL || wl->pointer == NULL)
    return (-1);
  lock = zwp_pointer_constraints_v1_lock_pointer(wl->pointer_constraints,
						  win->surface, wl->pointer, NULL,
						  ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT);
  if (lock == NULL)
    return (-1);
  activated = 0;
  zwp_locked_pointer_v1_add_listener(lock, &mlx__wayland_lock_listener, &activated);
  wl_display_roundtrip(wl->display);   /* wait for the 'locked' event, if any */
  if (activated == 0)
    {
      zwp_locked_pointer_v1_destroy(lock);
      return (-1);
    }
  zwp_locked_pointer_v1_set_cursor_position_hint(lock, wl_fixed_from_int(x),
						  wl_fixed_from_int(y));
  wl_surface_commit(win->surface);
  zwp_locked_pointer_v1_destroy(lock);   /* unlock: compositor may warp to the hint */
  wl_display_roundtrip(wl->display);
  return (0);
}
#endif


static int	mlx__wayland_extra_set_mouse(mlx__wayland_t *wl,
					     mlx__wayland_win_t *win,
					     int x, int y)
{
  static int	warned = 0;

#ifdef MLX_WAYLAND_HAVE_POINTER_WARP
  /* pointer-warp-v1 is a staging protocol (not part of core Wayland
     yet): available on recent Mutter/KWin, absent elsewhere */
  if (wl->pointer_warp && wl->pointer)
    {
      wp_pointer_warp_v1_warp_pointer(wl->pointer_warp, win->surface,
				       wl->pointer, wl_fixed_from_int(x),
				       wl_fixed_from_int(y),
				       wl->pointer_enter_serial);
      return (0);
    }
#endif
#ifdef MLX_WAYLAND_HAVE_POINTER_CONSTRAINTS
  if (mlx__wayland_extra_set_mouse_constraints(wl, win, x, y) == 0)
    return (0);
#endif
#if !defined(MLX_WAYLAND_HAVE_POINTER_WARP) && !defined(MLX_WAYLAND_HAVE_POINTER_CONSTRAINTS)
  (void)win; (void)x; (void)y;
#endif
  /* neither protocol is available (or the compositor supports
     neither): core Wayland has no other way for a client to move the
     cursor, only the compositor (or the user) can */
  (void)wl;
  if (warned == 0)
    {
      fprintf(stderr, "Mlx - Wayland: mlx_mouse_move() is not supported "
	      "(compositor lacks both pointer-warp-v1 and pointer-constraints)\n");
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
