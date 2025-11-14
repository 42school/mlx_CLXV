
#include	<sys/types.h>
#include	<stdlib.h>
#include	<string.h>

#include	"mlx_internal.h"

#include	<xcb/xcb.h>
#include	<xcb/xcb_keysyms.h>
#include	"mlx__xcb_internal.h"


static void	*mlx__xcb_window_error(mlx__xcb_t *xcb, mlx__xcb_win_t *win)
{
  if (xcb && win)
    {
      xcb_destroy_window(xcb->cnx, win->win_id);
      xcb_flush(xcb->cnx);   
      free(win);
    }
  return (NULL);
}



void	mlx__xcb_window_destroy(mlx_backend_hooks_param_t *param)
{
  mlx__xcb_t		*xcb;
  mlx__xcb_win_t	*win;
  
  xcb = (mlx__xcb_t *)(param->backend);
  win = (mlx__xcb_win_t *)(param->backend_win);
  mlx__xcb_window_error(xcb, win);
}



void	*mlx__xcb_window(mlx_backend_hooks_param_t *param)
{
  mlx__xcb_t		*xcb;
  mlx__xcb_win_t	*win;
  uint32_t		attributes[3];
  unsigned int		value;

  xcb = (mlx__xcb_t *)(param->backend);
  if ( (win = malloc(sizeof(*win))) == NULL )
    return (NULL);
  bzero(win->hook, MLX_XCB_MAX_EVENT * sizeof(*(win->hook)));
  bzero(win->hook_param, MLX_XCB_MAX_EVENT * sizeof(*(win->hook_param)));
  win->win_id = xcb_generate_id(xcb->cnx);
  win->width = param->size.width;
  win->height = param->size.height;
  win->xcb = xcb;
  win->mask = XCB_EVENT_MASK_EXPOSURE;
  attributes[0] = 0;
  attributes[1] = 0xFFFFFFFF;
  attributes[2] = win->mask;
  xcb_create_window(xcb->cnx,                      /* Connection          */
		    XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
		    win->win_id,                   /* window Id           */
		    xcb->screen->root,             /* parent window       */
		    0, 0,                          /* x, y                */
		    param->size.width, param->size.height,   /* width, height       */
		    10,                            /* border_width        */
		    XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
		    xcb->screen->root_visual,      /* visual              */
		    XCB_CW_BACK_PIXEL|XCB_CW_BORDER_PIXEL|XCB_CW_EVENT_MASK,
		    attributes);                   /* mask & values       */

  /* Set the title of the window */
  xcb_change_property (xcb->cnx, XCB_PROP_MODE_REPLACE, win->win_id,
		       XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
		       strlen(param->title), param->title);
  /* Set the title of the window icon */
  xcb_change_property (xcb->cnx, XCB_PROP_MODE_REPLACE, win->win_id,
		       XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8,
		       strlen(param->title), param->title);
  /* anti-resize window */
  mlx__xcb_anti_resize_win(xcb, win);
  /* get WM delete request */
  mlx__xcb_new_window_protocols(xcb, win);

  xcb_map_window(xcb->cnx, win->win_id);

  value = xcb->default_cursor;
  xcb_change_window_attributes(xcb->cnx, win->win_id, XCB_CW_CURSOR, &value);
  
  return (win);
}
