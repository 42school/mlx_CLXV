
/* mlx__appkit_internal.h */

#ifndef MLX__APPKIT_INTERNAL_H
#define MLX__APPKIT_INTERNAL_H

#include	<stdint.h>
#include	<vulkan/vulkan.h>
#include	<vulkan/vulkan_metal.h>

#ifdef __OBJC__
# import	<Cocoa/Cocoa.h>
# import	<QuartzCore/QuartzCore.h>
@class		MLXView;
@class		MLXWindowDelegate;
#else
typedef void	MLXView;
typedef void	MLXWindowDelegate;
typedef void	NSWindow;
#endif

/*
** AppKit has no notion of numbered protocol events the way X11/XCB
** does, but mlx_hook()'s raw event-number path is inherently backend-
** native (it forwards whatever number the caller passes straight to
** the backend) - mirror the exact xcb/xproto.h values, same as the
** Wayland backend does, so advanced code written against XCB's raw
** numbers (e.g. mlx_hook(win, 33, 0, ...) for WM_DELETE_WINDOW) keeps
** working unchanged on this backend too.
*/
#define	MLX_APPKIT_MAX_EVENT		34

#define	MLX_AK_EVENT_KEY_PRESS		2	/* XCB_KEY_PRESS */
#define	MLX_AK_EVENT_KEY_RELEASE	3	/* XCB_KEY_RELEASE */
#define	MLX_AK_EVENT_BUTTON_PRESS	4	/* XCB_BUTTON_PRESS */
#define	MLX_AK_EVENT_BUTTON_RELEASE	5	/* XCB_BUTTON_RELEASE */
#define	MLX_AK_EVENT_MOTION		6	/* XCB_MOTION_NOTIFY */
#define	MLX_AK_EVENT_EXPOSE		12	/* XCB_EXPOSE */
#define	MLX_AK_EVENT_CLOSE		33	/* XCB_CLIENT_MESSAGE (WM_DELETE_WINDOW) */


typedef struct			mlx__appkit_win_s	mlx__appkit_win_t;

/* one pending event, queued by the NSView/NSWindowDelegate callbacks
   and drained by mlx__appkit_event_get()/mlx__appkit_event_handle() -
   'win' is compared by pointer, same design as the Wayland backend */
typedef struct			mlx__appkit_event_s
{
  struct mlx__appkit_event_s	*next;
  mlx__appkit_win_t		*win;
  uint8_t			type;
  unsigned int			a;   /* keysym or button */
  unsigned int			x;
  unsigned int			y;
} mlx__appkit_event_t;


typedef struct			mlx__appkit_s
{
  unsigned int			screen_width;
  unsigned int			screen_height;
  int				cursor_visible;
  int				key_autorepeat;   /* filters queued KEY_PRESS, not the OS-level repeat itself */

  mlx__appkit_event_t		*queue_head;
  mlx__appkit_event_t		*queue_tail;
} mlx__appkit_t;


struct				mlx__appkit_win_s
{
  mlx__appkit_t			*ak;
  NSWindow			*window;
  MLXView			*view;
  MLXWindowDelegate		*delegate;
  CAMetalLayer			*metal_layer;
  unsigned int			width;
  unsigned int			height;
  int				ptr_x;
  int				ptr_y;
  int				configured;
  int				(*hook[MLX_APPKIT_MAX_EVENT])();
  void				*hook_param[MLX_APPKIT_MAX_EVENT];
};


/* internal prototypes */

void	mlx__appkit_queue_push(mlx__appkit_t *ak, mlx__appkit_win_t *win,
				uint8_t type, unsigned int a,
				unsigned int x, unsigned int y);
void	mlx__appkit_queue_purge_win(mlx__appkit_t *ak, mlx__appkit_win_t *win);
unsigned int	mlx__appkit_keysym(unsigned short keycode, unsigned int unichar);

#ifdef __OBJC__
/* NSEvent -> mlx__appkit_queue_push() translation, called directly
   from MLXView's overridden event methods (mlx__appkit_window.m),
   defined in mlx__appkit_event.m */
void	mlx__appkit_on_button(mlx__appkit_win_t *win, NSEvent *event, uint8_t type);
void	mlx__appkit_on_motion(mlx__appkit_win_t *win, NSEvent *event);
void	mlx__appkit_on_scroll(mlx__appkit_win_t *win, NSEvent *event);
void	mlx__appkit_on_key(mlx__appkit_win_t *win, NSEvent *event, uint8_t type);
#endif

#endif /* MLX__APPKIT_INTERNAL_H */
