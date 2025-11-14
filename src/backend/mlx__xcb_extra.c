
#include	"mlx_internal.h"

#include	<xcb/xcb.h>
#include	<xcb/xcb_keysyms.h>
#include	"mlx__xcb_internal.h"


static int	mlx__xcb_extra_keyrepeat(mlx__xcb_t *xcb, int action)
{
  unsigned int values;
  
  values = action?XCB_AUTO_REPEAT_MODE_ON:XCB_AUTO_REPEAT_MODE_OFF;
  xcb_change_keyboard_control(xcb->cnx, XCB_KB_AUTO_REPEAT_MODE, &values);
  return (0);
}


static int	mlx__xcb_extra_showhide(mlx__xcb_t *xcb, mlx__xcb_win_t *win, int action)
{
  unsigned int				value;

  if (action == 0)
    value = xcb->empty_cursor;
  else
    value = xcb->default_cursor;
  xcb_change_window_attributes(xcb->cnx, win->win_id, XCB_CW_CURSOR, &value);
  
  xcb_flush(xcb->cnx);
  return (0);
}


static int	mlx__xcb_extra_get_mouse(mlx__xcb_t *xcb, mlx__xcb_win_t *win,
					 int *x, int *y)
{
  xcb_query_pointer_cookie_t cookie;
  xcb_query_pointer_reply_t *reply;

  cookie = xcb_query_pointer(xcb->cnx, win->win_id);
  // ?? need flush here ??
  reply = xcb_query_pointer_reply(xcb->cnx, cookie, NULL);
  *x = reply->win_x;
  *y = reply->win_y;
  return (0);
}


static int	mlx__xcb_extra_set_mouse(mlx__xcb_t *xcb, mlx__xcb_win_t *win,
					 int x, int y)
{
  xcb_warp_pointer(xcb->cnx, XCB_NONE, win->win_id, 0, 0, 0, 0, x, y);
  xcb_flush(xcb->cnx);
  return (0);
}



int	mlx__xcb_extra(mlx_backend_hooks_param_t *param)
{
  mlx__xcb_t		*mxcb;
  mlx__xcb_win_t	*win;

  mxcb = (mlx__xcb_t *)param->backend;
  win = (mlx__xcb_win_t *)param->backend_win;

  if (param->action == MLX_BE_MOUSE_HIDE)
    return (mlx__xcb_extra_showhide(mxcb, win, 0));
  if (param->action == MLX_BE_MOUSE_SHOW)
    return (mlx__xcb_extra_showhide(mxcb, win, 1));
  if (param->action == MLX_BE_KEY_AUTO_OFF)
    return (mlx__xcb_extra_keyrepeat(mxcb, 0));
  if (param->action == MLX_BE_KEY_AUTO_ON)
    return (mlx__xcb_extra_keyrepeat(mxcb, 1));
  if (param->action == MLX_BE_MOUSE_GET)
    return (mlx__xcb_extra_get_mouse(mxcb, win, &(param->size.x),
				     &(param->size.y)));
  if (param->action == MLX_BE_MOUSE_SET)
    return (mlx__xcb_extra_set_mouse(mxcb, win, param->size.x, param->size.y));
  return (-1);
}
