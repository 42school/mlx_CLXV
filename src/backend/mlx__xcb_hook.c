
#include	<stdio.h>

#include	<xcb/xcb.h>
#include	<xcb/xcb_keysyms.h>

#include	"mlx_internal.h"
#include	"mlx__xcb_internal.h"


void	mlx__xcb_update_mask(mlx__xcb_win_t *win, int mask, int action)
{
  if (action)
    win->mask |= mask;
  else
    win->mask &= ~mask;
  xcb_change_window_attributes(win->xcb->cnx, win->win_id,
			       XCB_CW_EVENT_MASK, &(win->mask));
}

int	mlx__xcb_convert_event[4] =
  {0, XCB_KEY_RELEASE, XCB_BUTTON_PRESS, XCB_EXPOSE};
int	mlx__xcb_convert_event_mask[4] =
  {0, XCB_EVENT_MASK_KEY_RELEASE, XCB_EVENT_MASK_BUTTON_PRESS,
   XCB_EVENT_MASK_EXPOSURE};


int	mlx__xcb_event_hook(mlx_backend_hooks_param_t *param)
{
  mlx__xcb_win_t *win;

  win = (mlx__xcb_win_t *)(param->backend_win);
  if (param->mlx_event > 0)
    {
      param->backend_event_number = mlx__xcb_convert_event[param->mlx_event];
      param->backend_event_mask =
	mlx__xcb_convert_event_mask[param->mlx_event];
    }
  
  win->hook[param->backend_event_number] = param->backend_event_hook;
  win->hook_param[param->backend_event_number] = param->backend_event_param;

  mlx__xcb_update_mask(win, param->backend_event_mask,
		       param->backend_event_hook == NULL ? 0 : 1);
  return (0);
}
