

#include	<stdlib.h>
#include	<stddef.h>

#include	<stdio.h>

#include	"mlx_internal.h"

#include        <xcb/xcb.h>
#include	<xcb/xcb_keysyms.h>

#include        "mlx__xcb_internal.h"
#include	"mlx__xcb_event.h"



void			mlx__xcb_event_handle(mlx_backend_hooks_param_t *param)
{
  mlx__xcb_t		*xcb;
  mlx__xcb_win_t	*win;
  xcb_window_t		winid;
  xcb_generic_event_t	*event;

  xcb = (mlx__xcb_t *)(param->backend);
  win = (mlx__xcb_win_t *)(param->backend_win);
  event = (xcb_generic_event_t *)(param->event);

  if (win == NULL) // no more win to compare with
    {
      free(event);
      return ;
    }
  
  event->response_type &= ~0x80; /* remove SendEvent bit */
  if (event->response_type == 0) /* actually xcb error */
    return ;

  if (mlx__xcb_event_tab[event->response_type].wpos > 0)
    {
      winid = *((xcb_window_t *)( ((unsigned char *)event)+mlx__xcb_event_tab[event->response_type].wpos ));
      if (winid == win->win_id && win->hook[event->response_type] != NULL)
	mlx__xcb_event_tab[event->response_type].handle(xcb, win, event);
      return ;
    }
  if (win->hook[event->response_type] != NULL)
    mlx__xcb_event_tab[event->response_type].handle(xcb, win, event);
}



void	*mlx__xcb_event_get(mlx_backend_hooks_param_t *param)
{
  mlx__xcb_t		*xcb;

  xcb = (mlx__xcb_t *)(param->backend);
  if (param->backend_event_block)
    return (xcb_wait_for_event(xcb->cnx));
  return (xcb_poll_for_event(xcb->cnx));
}
