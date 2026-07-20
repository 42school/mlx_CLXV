
#include	<stdlib.h>

#include	"mlx_internal.h"
#include	"mlx__appkit_internal.h"


void	mlx__appkit_queue_push(mlx__appkit_t *ak, mlx__appkit_win_t *win,
				uint8_t type, unsigned int a,
				unsigned int x, unsigned int y)
{
  mlx__appkit_event_t	*ev;

  if ((ev = malloc(sizeof(*ev))) == NULL)
    return ;
  ev->next = NULL;
  ev->win = win;
  ev->type = type;
  ev->a = a;
  ev->x = x;
  ev->y = y;
  if (ak->queue_tail)
    ak->queue_tail->next = ev;
  else
    ak->queue_head = ev;
  ak->queue_tail = ev;
}

/* drop any already-queued events still pointing at a window that is
   about to be freed, mirrors the same fix in the Wayland backend */
void	mlx__appkit_queue_purge_win(mlx__appkit_t *ak, mlx__appkit_win_t *win)
{
  mlx__appkit_event_t	head;
  mlx__appkit_event_t	*chain;
  mlx__appkit_event_t	*drop;

  head.next = ak->queue_head;
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
  ak->queue_head = head.next;
  ak->queue_tail = chain == &head ? NULL : chain;
}
