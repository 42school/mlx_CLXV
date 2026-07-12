
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
** Wayland has no notion of these as numbered protocol opcodes the way
** X11/XCB does, but mlx_hook()'s raw event-number path is inherently
** backend-native (it forwards whatever number the student passes
** straight to the backend) - so advanced code written against the XCB
** numbers (e.g. mlx_hook(win, 33, 0, ...) for WM_DELETE_WINDOW) keeps
** working unchanged on this backend too, these mirror the exact values
** from xcb/xproto.h rather than inventing a separate numbering.
*/
#define	MLX_WAYLAND_MAX_EVENT		34

#define	MLX_WL_EVENT_KEY_PRESS		2	/* XCB_KEY_PRESS */
#define	MLX_WL_EVENT_KEY_RELEASE	3	/* XCB_KEY_RELEASE */
#define	MLX_WL_EVENT_BUTTON_PRESS	4	/* XCB_BUTTON_PRESS */
#define	MLX_WL_EVENT_BUTTON_RELEASE	5	/* XCB_BUTTON_RELEASE */
#define	MLX_WL_EVENT_MOTION		6	/* XCB_MOTION_NOTIFY */
#define	MLX_WL_EVENT_EXPOSE		12	/* XCB_EXPOSE */
#define	MLX_WL_EVENT_CLOSE		33	/* XCB_CLIENT_MESSAGE (WM_DELETE_WINDOW) */

/* height, in pixels, of the fake title bar mlx__wayland_wm.c draws
   when the compositor offers no server-side decoration; the title
   text itself is still blitted at font_atlas.height's native size
   (mlx_font.c), just with a bit of vertical padding around it */
#define	MLX_WM_TITLEBAR_HEIGHT		24
#define	MLX_WM_TITLEBAR_CLOSE_WIDTH	24


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
  struct wl_subcompositor	*subcompositor;
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

  /* raw currently-entered surface: wl_pointer.motion/button don't repeat
     it, only .enter does, so it must be tracked to know whether motion/
     button targets a real mlx window (pointer_focus) or the fake
     title bar surface (mlx_wm.c) */
  struct wl_surface		*pointer_surface;
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
  char				*title;   /* copy, needed by mlx__wayland_wm.c */
  int				configured;
  int				ptr_x;
  int				ptr_y;
  int				(*hook[MLX_WAYLAND_MAX_EVENT])();
  void				*hook_param[MLX_WAYLAND_MAX_EVENT];

  /* fake title bar (mlx__wayland_wm.c), only created when the compositor
     grants no server-side decoration for this window */
  struct wl_surface		*titlebar_surface;
  struct wl_subsurface		*titlebar_subsurface;
  struct wl_buffer		*titlebar_buffer;
  int				titlebar_ptr_x;
  int				titlebar_ptr_y;
};


/* internal prototypes */

void	mlx__wayland_queue_push(mlx__wayland_t *wl, mlx__wayland_win_t *win,
				 uint8_t type, unsigned int a,
				 unsigned int x, unsigned int y);
void	mlx__wayland_seat_bind(mlx__wayland_t *wl);
int	mlx__wayland_seat_init(mlx__wayland_t *wl);
void	mlx__wayland_seat_destroy(mlx__wayland_t *wl);
int	mlx__wayland_cursor_init(mlx__wayland_t *wl);
void	mlx__wayland_cursor_destroy(mlx__wayland_t *wl);
void	mlx__wayland_cursor_set(mlx__wayland_t *wl, int visible);
void	mlx__wayland_win_add(mlx__wayland_t *wl, mlx__wayland_win_t *win);
void	mlx__wayland_win_remove(mlx__wayland_t *wl, mlx__wayland_win_t *win);

/* mlx__wayland_wm.c - the fake title bar used when no server-side
   decoration is available; entirely self-contained, the rest of the
   backend only needs to create/destroy it and forward pointer events */
void	mlx__wayland_wm_titlebar_create(mlx__wayland_win_t *win);
void	mlx__wayland_wm_titlebar_destroy(mlx__wayland_win_t *win);
int	mlx__wayland_wm_pointer_enter(mlx__wayland_t *wl, struct wl_surface *surface);
int	mlx__wayland_wm_pointer_leave(mlx__wayland_t *wl, struct wl_surface *surface);
int	mlx__wayland_wm_pointer_motion(mlx__wayland_t *wl, struct wl_surface *surface,
					int x, int y);
int	mlx__wayland_wm_pointer_button(mlx__wayland_t *wl, struct wl_surface *surface,
					uint32_t serial, uint32_t button, uint32_t state);


#endif /* MLX__WAYLAND_INTERNAL_H */
