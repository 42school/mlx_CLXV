
#include	<stdlib.h>
#include	<string.h>

#include	<xcb/xcb.h>
#include	<xcb/xcb_keysyms.h>
#include	"mlx__xcb_internal.h"

#include	"mlx__xcb_wmhints.h"


void	mlx__xcb_anti_resize_win(mlx__xcb_t *xcb, mlx__xcb_win_t *win)
{
  struct WMSizeHints hints =
    {
      .flags = WM_SIZE_HINT_P_SIZE | WM_SIZE_HINT_P_MIN_SIZE |
      WM_SIZE_HINT_P_MAX_SIZE,
      .width = win->width,
      .height = win->height,
      .min_width = win->width,
      .min_height = win->height,
      .max_width = win->width,
      .max_height = win->height
    };

 xcb_change_property(xcb->cnx, XCB_PROP_MODE_REPLACE, win->win_id,
		     XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS,
		     32, sizeof(struct WMSizeHints) >> 2, &hints);
}


void	mlx__xcb_new_window_protocols(mlx__xcb_t *xcb, mlx__xcb_win_t *win)
{
  xcb_intern_atom_cookie_t wmDeleteCookie;
  xcb_intern_atom_cookie_t wmProtocolsCookie;
  xcb_intern_atom_reply_t *wmDeleteReply;
  xcb_intern_atom_reply_t *wmProtocolsReply;

  wmDeleteCookie = xcb_intern_atom(xcb->cnx, 0, strlen("WM_DELETE_WINDOW"),
				   "WM_DELETE_WINDOW");
  wmProtocolsCookie = xcb_intern_atom(xcb->cnx, 0, strlen("WM_PROTOCOLS"),
				      "WM_PROTOCOLS");
  wmDeleteReply = xcb_intern_atom_reply(xcb->cnx, wmDeleteCookie, NULL);
  wmProtocolsReply = xcb_intern_atom_reply(xcb->cnx, wmProtocolsCookie, NULL);

  xcb_change_property(xcb->cnx, XCB_PROP_MODE_REPLACE, win->win_id,
		      wmProtocolsReply->atom, 4, 32, 1, &wmDeleteReply->atom);
  free(wmDeleteReply);
  free(wmProtocolsReply);
}
