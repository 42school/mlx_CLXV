
#include	<stdlib.h>

#include	"mlx_internal.h"
#include	"mlx__appkit_internal.h"

#import	<Cocoa/Cocoa.h>

/* macOS virtual keycodes (HIToolbox/Events.h's kVK_* constants,
   hardcoded here to avoid a Carbon.framework header dependency) ->
   X11 keysyms (X11/keysymdef.h), for the keys that have no printable
   representation reachable through charactersIgnoringModifiers */
static const struct
{
  unsigned short	keycode;
  unsigned int		keysym;
}				mlx__appkit_keytable[] =
{
  {0x24, 0xFF0D},   /* Return */
  {0x30, 0xFF09},   /* Tab */
  {0x33, 0xFF08},   /* Delete (backspace) */
  {0x35, 0xFF1B},   /* Escape */
  {0x37, 0xFFEB},   /* Command -> Super_L */
  {0x38, 0xFFE1},   /* Shift -> Shift_L */
  {0x39, 0xFFE5},   /* CapsLock */
  {0x3A, 0xFFE9},   /* Option -> Alt_L */
  {0x3B, 0xFFE3},   /* Control -> Control_L */
  {0x3C, 0xFFE2},   /* RightShift -> Shift_R */
  {0x3D, 0xFFEA},   /* RightOption -> Alt_R */
  {0x3E, 0xFFE4},   /* RightControl -> Control_R */
  {0x73, 0xFF50},   /* Home */
  {0x74, 0xFF55},   /* PageUp */
  {0x75, 0xFFFF},   /* ForwardDelete */
  {0x77, 0xFF57},   /* End */
  {0x79, 0xFF56},   /* PageDown */
  {0x7B, 0xFF51},   /* Left */
  {0x7C, 0xFF53},   /* Right */
  {0x7D, 0xFF54},   /* Down */
  {0x7E, 0xFF52},   /* Up */
  {0x7A, 0xFFBE},   /* F1 */
  {0x78, 0xFFBF},   /* F2 */
  {0x63, 0xFFC0},   /* F3 */
  {0x76, 0xFFC1},   /* F4 */
  {0x60, 0xFFC2},   /* F5 */
  {0x61, 0xFFC3},   /* F6 */
  {0x62, 0xFFC4},   /* F7 */
  {0x64, 0xFFC5},   /* F8 */
  {0x65, 0xFFC6},   /* F9 */
  {0x6D, 0xFFC7},   /* F10 */
  {0x67, 0xFFC8},   /* F11 */
  {0x6F, 0xFFC9},   /* F12 */
};

unsigned int	mlx__appkit_keysym(unsigned short keycode, unsigned int unichar)
{
  size_t	i;

  i = 0;
  while (i < sizeof(mlx__appkit_keytable) / sizeof(mlx__appkit_keytable[0]))
    {
      if (mlx__appkit_keytable[i].keycode == keycode)
	return (mlx__appkit_keytable[i].keysym);
      i++;
    }
  /* printable ASCII/Latin-1: the character's own Unicode codepoint is
     already its X11 keysym value in this range */
  if (unichar >= 0x20 && unichar <= 0xFF)
    return (unichar);
  return (0);
}

static unsigned int	mlx__appkit_button_number(NSEvent *event)
{
  switch (event.buttonNumber)
    {
    case 0:
      return (1);   /* left */
    case 1:
      return (3);   /* right */
    case 2:
      return (2);   /* middle */
    default:
      return ((unsigned int)event.buttonNumber + 1);
    }
}

static void	mlx__appkit_update_ptr(mlx__appkit_win_t *win, NSEvent *event)
{
  NSPoint	p;

  p = [win->view convertPoint:event.locationInWindow fromView:nil];
  win->ptr_x = (int)p.x;
  win->ptr_y = (int)p.y;
}

void	mlx__appkit_on_button(mlx__appkit_win_t *win, NSEvent *event, uint8_t type)
{
  mlx__appkit_update_ptr(win, event);
  mlx__appkit_queue_push(win->ak, win, type, mlx__appkit_button_number(event),
			  (unsigned int)win->ptr_x, (unsigned int)win->ptr_y);
}

void	mlx__appkit_on_motion(mlx__appkit_win_t *win, NSEvent *event)
{
  mlx__appkit_update_ptr(win, event);
  mlx__appkit_queue_push(win->ak, win, MLX_AK_EVENT_MOTION, 0,
			  (unsigned int)win->ptr_x, (unsigned int)win->ptr_y);
}

void	mlx__appkit_on_scroll(mlx__appkit_win_t *win, NSEvent *event)
{
  unsigned int	button;

  if (event.deltaY == 0)
    return ;
  button = event.deltaY > 0 ? 4 : 5;   /* mirrors X11: scroll up=4, down=5 */
  mlx__appkit_queue_push(win->ak, win, MLX_AK_EVENT_BUTTON_PRESS, button,
			  (unsigned int)win->ptr_x, (unsigned int)win->ptr_y);
  mlx__appkit_queue_push(win->ak, win, MLX_AK_EVENT_BUTTON_RELEASE, button,
			  (unsigned int)win->ptr_x, (unsigned int)win->ptr_y);
}

void	mlx__appkit_on_key(mlx__appkit_win_t *win, NSEvent *event, uint8_t type)
{
  NSString	*chars;
  unichar	c;

  if (type == MLX_AK_EVENT_KEY_PRESS && event.isARepeat && !win->ak->key_autorepeat)
    return ;
  chars = event.charactersIgnoringModifiers;
  c = chars.length > 0 ? [chars characterAtIndex:0] : 0;
  mlx__appkit_queue_push(win->ak, win, type,
			  mlx__appkit_keysym(event.keyCode, (unsigned int)c), 0, 0);
}

static mlx__appkit_event_t	*mlx__appkit_queue_pop(mlx__appkit_t *ak)
{
  mlx__appkit_event_t	*ev;

  ev = ak->queue_head;
  if (ev == NULL)
    return (NULL);
  ak->queue_head = ev->next;
  if (ak->queue_head == NULL)
    ak->queue_tail = NULL;
  return (ev);
}

void	*mlx__appkit_event_get(mlx_backend_hooks_param_t *param)
{
  mlx__appkit_t	*ak;
  NSEvent	*event;
  NSDate	*until;

  ak = (mlx__appkit_t *)(param->backend);
  if (ak->queue_head)
    return (mlx__appkit_queue_pop(ak));
  @autoreleasepool
    {
      until = param->backend_event_block ? [NSDate distantFuture] : [NSDate distantPast];
      event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:until
				     inMode:NSDefaultRunLoopMode dequeue:YES];
      if (event)
	[NSApp sendEvent:event];
    }
  return (mlx__appkit_queue_pop(ak));
}

void	mlx__appkit_event_handle(mlx_backend_hooks_param_t *param)
{
  mlx__appkit_win_t	*win;
  mlx__appkit_event_t	*ev;
  int			(*f_key)(unsigned int, void *);
  int			(*f_button)(unsigned int, unsigned int, unsigned int, void *);
  int			(*f_motion)(unsigned int, unsigned int, void *);
  int			(*f_generic)(void *);

  win = (mlx__appkit_win_t *)(param->backend_win);
  ev = (mlx__appkit_event_t *)(param->event);
  if (win == NULL)   /* final call of the round: free the event */
    {
      free(ev);
      return ;
    }
  if (ev->win != win || win->hook[ev->type] == NULL)
    return ;
  if (ev->type == MLX_AK_EVENT_KEY_PRESS || ev->type == MLX_AK_EVENT_KEY_RELEASE)
    {
      f_key = win->hook[ev->type];
      f_key(ev->a, win->hook_param[ev->type]);
    }
  else if (ev->type == MLX_AK_EVENT_BUTTON_PRESS || ev->type == MLX_AK_EVENT_BUTTON_RELEASE)
    {
      f_button = win->hook[ev->type];
      f_button(ev->a, ev->x, ev->y, win->hook_param[ev->type]);
    }
  else if (ev->type == MLX_AK_EVENT_MOTION)
    {
      f_motion = win->hook[ev->type];
      f_motion(ev->x, ev->y, win->hook_param[ev->type]);
    }
  else
    {
      f_generic = win->hook[ev->type];
      f_generic(win->hook_param[ev->type]);
    }
}
