
/* mlx__wayland_internal.h */

#ifndef MLX__WAYLAND_INTERNAL_H
#define MLX__WAYLAND_INTERNAL_H

#include	<stdint.h>
#include	<wayland-client.h>
#include	<wayland-cursor.h>
#include	<xkbcommon/xkbcommon.h>

#include	"mlx__wayland_xdg_shell_protocol.h"
#ifdef MLX_WAYLAND_HAVE_POINTER_WARP
# include	"mlx__wayland_pointer_warp_protocol.h"
#endif
#ifdef MLX_WAYLAND_HAVE_DECORATION
# include	"mlx__wayland_decoration_protocol.h"
#endif

/*
** MLX has no notion of native Wayland event numbers (unlike X11/XCB),
** so this backend defines its own small event space instead of reusing
** raw protocol opcodes. Advanced users relying on the generic mlx_hook()
** with raw XCB event numbers will need this set of numbers instead when
** running on the Wayland backend.
*/
#define	MLX_WAYLAND_MAX_EVENT		8

#define	MLX_WL_EVENT_KEY_PRESS		1
#define	MLX_WL_EVENT_KEY_RELEASE	2
#define	MLX_WL_EVENT_BUTTON_PRESS	3
#define	MLX_WL_EVENT_BUTTON_RELEASE	4
#define	MLX_WL_EVENT_MOTION		5
#define	MLX_WL_EVENT_EXPOSE		6
#define	MLX_WL_EVENT_CLOSE		7


typedef struct			mlx__wayland_win_s	mlx__wayland_win_t;


/* one pending event, queued by the seat listeners and drained by
   mlx__wayland_event_get() / mlx__wayland_event_handle() -
   'win' is compared by pointer, no offset lookup needed like in XCB */
typedef struct			mlx__wayland_event_s
{
  struct mlx__wayland_event_s	*next;
  mlx__wayland_win_t		*win;
  uint8_t			type;
  unsigned int			a;   /* keysym or button */
  unsigned int			x;
  unsigned int			y;
} mlx__wayland_event_t;


typedef struct			mlx__wayland_s
{
  struct wl_display		*display;
  struct wl_registry		*registry;
  struct wl_compositor		*compositor;
  struct wl_shm			*shm;
  struct wl_seat		*seat;
  struct wl_pointer		*pointer;
  struct wl_keyboard		*keyboard;
  struct wl_output		*output;
  struct xdg_wm_base		*xdg_wm_base;
#ifdef MLX_WAYLAND_HAVE_POINTER_WARP
  struct wp_pointer_warp_v1	*pointer_warp;
#endif
#ifdef MLX_WAYLAND_HAVE_DECORATION
  struct zxdg_decoration_manager_v1	*decoration_manager;
#endif

  struct xkb_context		*xkb_context;
  struct xkb_keymap		*xkb_keymap;
  struct xkb_state		*xkb_state;

  struct wl_cursor_theme	*cursor_theme;
  struct wl_cursor		*cursor_default;
  struct wl_surface		*cursor_surface;
  unsigned int			pointer_enter_serial;
  int				cursor_visible;

  mlx__wayland_win_t		*pointer_focus;
  mlx__wayland_win_t		*keyboard_focus;

  mlx__wayland_win_t		*windows;   /* all backend windows, for pointer/kbd lookup */

  mlx__wayland_event_t		*queue_head;
  mlx__wayland_event_t		*queue_tail;

  unsigned int			screen_width;
  unsigned int			screen_height;
} mlx__wayland_t;


struct				mlx__wayland_win_s
{
  mlx__wayland_win_t		*next;
  mlx__wayland_t		*wl;
  struct wl_surface		*surface;
  struct xdg_surface		*xdg_surface;
  struct xdg_toplevel		*xdg_toplevel;
#ifdef MLX_WAYLAND_HAVE_DECORATION
  struct zxdg_toplevel_decoration_v1	*decoration;
#endif
  unsigned int			width;
  unsigned int			height;
  int				configured;
  int				ptr_x;
  int				ptr_y;
  int				(*hook[MLX_WAYLAND_MAX_EVENT])();
  void				*hook_param[MLX_WAYLAND_MAX_EVENT];
};


/* internal prototypes */

void	mlx__wayland_queue_push(mlx__wayland_t *wl, mlx__wayland_win_t *win,
				 uint8_t type, unsigned int a,
				 unsigned int x, unsigned int y);
int	mlx__wayland_seat_init(mlx__wayland_t *wl);
void	mlx__wayland_seat_destroy(mlx__wayland_t *wl);
int	mlx__wayland_cursor_init(mlx__wayland_t *wl);
void	mlx__wayland_cursor_destroy(mlx__wayland_t *wl);
void	mlx__wayland_cursor_set(mlx__wayland_t *wl, int visible);
void	mlx__wayland_win_add(mlx__wayland_t *wl, mlx__wayland_win_t *win);
void	mlx__wayland_win_remove(mlx__wayland_t *wl, mlx__wayland_win_t *win);


#endif /* MLX__WAYLAND_INTERNAL_H */
