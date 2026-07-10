
#include	<stdlib.h>
#include	<poll.h>

#include	"mlx_internal.h"

#include	<wayland-client.h>

#include	"mlx__wayland_internal.h"


static mlx__wayland_event_t	*mlx__wayland_event_pop(mlx__wayland_t *wl)
{
  mlx__wayland_event_t	*ev;

  ev = wl->queue_head;
  if (ev == NULL)
    return (NULL);
  wl->queue_head = ev->next;
  if (wl->queue_head == NULL)
    wl->queue_tail = NULL;
  return (ev);
}


void	*mlx__wayland_event_get(mlx_backend_hooks_param_t *param)
{
  mlx__wayland_t	*wl;
  struct pollfd		pfd;

  wl = (mlx__wayland_t *)(param->backend);

  /* drain what listeners already queued before touching the socket again */
  if (wl->queue_head)
    return (mlx__wayland_event_pop(wl));

  while (wl_display_prepare_read(wl->display) != 0)
    wl_display_dispatch_pending(wl->display);
  wl_display_flush(wl->display);

  pfd.fd = wl_display_get_fd(wl->display);
  pfd.events = POLLIN;
  pfd.revents = 0;
  if (poll(&pfd, 1, param->backend_event_block ? -1 : 0) <= 0)
    {
      wl_display_cancel_read(wl->display);
      return (NULL);
    }
  wl_display_read_events(wl->display);
  wl_display_dispatch_pending(wl->display);

  return (mlx__wayland_event_pop(wl));
}


void	mlx__wayland_event_handle(mlx_backend_hooks_param_t *param)
{
  mlx__wayland_win_t	*win;
  mlx__wayland_event_t	*ev;
  int			(*f)();

  win = (mlx__wayland_win_t *)(param->backend_win);
  ev = (mlx__wayland_event_t *)(param->event);

  if (win == NULL)   /* final call of the round: free the event */
    {
      free(ev);
      return ;
    }
  if (ev->win != win || win->hook[ev->type] == NULL)
    return ;
  f = win->hook[ev->type];
  if (ev->type == MLX_WL_EVENT_KEY_PRESS || ev->type == MLX_WL_EVENT_KEY_RELEASE)
    f(ev->a, win->hook_param[ev->type]);
  else if (ev->type == MLX_WL_EVENT_BUTTON_PRESS ||
	   ev->type == MLX_WL_EVENT_BUTTON_RELEASE)
    f(ev->a, ev->x, ev->y, win->hook_param[ev->type]);
  else if (ev->type == MLX_WL_EVENT_MOTION)
    f(ev->x, ev->y, win->hook_param[ev->type]);
  else
    f(win->hook_param[ev->type]);
}
