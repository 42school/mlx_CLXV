
#include	"mlx_internal.h"
#include	"mlx__appkit_internal.h"

#import	<Cocoa/Cocoa.h>
#import	<ApplicationServices/ApplicationServices.h>

static int	mlx__appkit_extra_showhide(mlx__appkit_t *ak, int visible)
{
  if (visible == 0 && ak->cursor_visible)
    {
      [NSCursor hide];
      ak->cursor_visible = 0;
    }
  else if (visible != 0 && !ak->cursor_visible)
    {
      [NSCursor unhide];
      ak->cursor_visible = 1;
    }
  return (0);
}

static int	mlx__appkit_extra_get_mouse(mlx__appkit_win_t *win, int *x, int *y)
{
  *x = win->ptr_x;
  *y = win->ptr_y;
  return (0);
}

static int	mlx__appkit_extra_set_mouse(mlx__appkit_win_t *win, int x, int y)
{
  NSPoint	win_pt;
  NSPoint	screen_pt;
  CGPoint	cg_pt;

  @autoreleasepool
    {
      /* win->view is flipped (top-left, y-down) to match mlx's own
	 convention, so converting up to window/screen space (both
	 bottom-left, y-up in AppKit) un-flips it automatically */
      win_pt = [win->view convertPoint:NSMakePoint(x, y) toView:nil];
      screen_pt = [win->window convertPointToScreen:win_pt];
      /* CGWarpMouseCursorPosition wants top-left-origin, y-down global
	 display coordinates, the opposite of AppKit's screen space */
      cg_pt.x = screen_pt.x;
      cg_pt.y = win->ak->screen_height - screen_pt.y;
      CGWarpMouseCursorPosition(cg_pt);
      CGAssociateMouseAndMouseCursorPosition(true);
    }
  win->ptr_x = x;
  win->ptr_y = y;
  return (0);
}

int	mlx__appkit_extra(mlx_backend_hooks_param_t *param)
{
  mlx__appkit_t		*ak;
  mlx__appkit_win_t	*win;

  ak = (mlx__appkit_t *)(param->backend);
  win = (mlx__appkit_win_t *)(param->backend_win);
  if (param->action == MLX_BE_MOUSE_HIDE)
    return (mlx__appkit_extra_showhide(ak, 0));
  if (param->action == MLX_BE_MOUSE_SHOW)
    return (mlx__appkit_extra_showhide(ak, 1));
  if (param->action == MLX_BE_MOUSE_GET)
    return (mlx__appkit_extra_get_mouse(win, &(param->size.x), &(param->size.y)));
  if (param->action == MLX_BE_MOUSE_SET)
    return (mlx__appkit_extra_set_mouse(win, param->size.x, param->size.y));
  if (param->action == MLX_BE_KEY_AUTO_OFF)
    {
      ak->key_autorepeat = 0;
      return (0);
    }
  if (param->action == MLX_BE_KEY_AUTO_ON)
    {
      ak->key_autorepeat = 1;
      return (0);
    }
  return (-1);
}
