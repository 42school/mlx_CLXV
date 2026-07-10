
#include	<stdlib.h>

#include	"mlx_internal.h"

#include	<wayland-client.h>

#include	"mlx__wayland_internal.h"


void	mlx__wayland_win_add(mlx__wayland_t *wl, mlx__wayland_win_t *win)
{
  win->next = wl->windows;
  wl->windows = win;
}

static void	mlx__wayland_queue_purge_win(mlx__wayland_t *wl,
					     mlx__wayland_win_t *win)
{
  mlx__wayland_event_t	head;
  mlx__wayland_event_t	*chain;
  mlx__wayland_event_t	*drop;

  head.next = wl->queue_head;
  chain = &head;
  while (chain->next)
    {
      if (chain->next->win == win)
	{
	  drop = chain->next;
	  chain->next = drop->next;
	  free(drop);
	}
      else
	chain = chain->next;
    }
  wl->queue_head = head.next;
  wl->queue_tail = chain == &head ? NULL : chain;
}

void	mlx__wayland_win_remove(mlx__wayland_t *wl, mlx__wayland_win_t *win)
{
  mlx__wayland_win_t	head;
  mlx__wayland_win_t	*chain;

  head.next = wl->windows;
  chain = &head;
  while (chain->next && chain->next != win)
    chain = chain->next;
  if (chain->next == win)
    chain->next = win->next;
  wl->windows = head.next;
  if (wl->pointer_focus == win)
    wl->pointer_focus = NULL;
  if (wl->keyboard_focus == win)
    wl->keyboard_focus = NULL;
  /* drop any already-queued events still pointing at this window,
     it is about to be freed by the caller */
  mlx__wayland_queue_purge_win(wl, win);
}


void	mlx__wayland_queue_push(mlx__wayland_t *wl, mlx__wayland_win_t *win,
				 uint8_t type, unsigned int a,
				 unsigned int x, unsigned int y)
{
  mlx__wayland_event_t	*ev;

  if ((ev = malloc(sizeof(*ev))) == NULL)
    return ;
  ev->next = NULL;
  ev->win = win;
  ev->type = type;
  ev->a = a;
  ev->x = x;
  ev->y = y;
  if (wl->queue_tail)
    wl->queue_tail->next = ev;
  else
    wl->queue_head = ev;
  wl->queue_tail = ev;
}
