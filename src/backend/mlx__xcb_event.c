

#include	<xcb/xcb.h>
#include	<xcb/xcb_keysyms.h>

#include	"mlx__xcb_internal.h"


// works for key press & key release, xcb struct is identical
void mlx__xcb_event_key(mlx__xcb_t *mlx_xcb, mlx__xcb_win_t *win,
			xcb_key_press_event_t *ev)
{
  xcb_keysym_t ks;
  int	(*f)(unsigned int, void *, xcb_key_press_event_t *ev);

  f = win->hook[ev->response_type];
  ks = xcb_key_press_lookup_keysym(mlx_xcb->keysymbols, ev, 0);
  f((unsigned int)ks, win->hook_param[ev->response_type], ev);
}

void mlx__xcb_event_mouse(mlx__xcb_t *mlx_xcb, mlx__xcb_win_t *win,
			  xcb_key_press_event_t *ev)
{
  int	(*f)(unsigned int, unsigned int, unsigned int, void *, xcb_key_press_event_t *ev);

  f = win->hook[ev->response_type];
  f((unsigned int)ev->detail, (unsigned int)ev->event_x,
    (unsigned int)ev->event_y, win->hook_param[ev->response_type], ev);
}

void mlx__xcb_event_motion(mlx__xcb_t *mlx_xcb, mlx__xcb_win_t *win,
			   xcb_key_press_event_t *ev)
{
  int	(*f)(unsigned int, unsigned int, void *, xcb_key_press_event_t *ev);

  f = win->hook[ev->response_type];
  f((unsigned int)ev->event_x, (unsigned int)ev->event_y,
    win->hook_param[ev->response_type], ev);
}


void mlx__xcb_event_generic(mlx__xcb_t *mlx_xcb, mlx__xcb_win_t *win,
			    xcb_key_press_event_t *ev)
{
  int	(*f)(void *, xcb_key_press_event_t *ev);

  f = win->hook[ev->response_type];
  f(win->hook_param[ev->response_type], ev);
}
