
#include	<stdlib.h>
#include	<stdio.h>

#include	"mlx_internal.h"

#include	<xcb/xcb.h>
#include	<xcb/xcb_keysyms.h>

#include	"backend/mlx__xcb_internal.h"


static void	*mlx__xcb_error(mlx__xcb_t *mxcb)
{
  if (mxcb)
    {
      xcb_free_cursor(mxcb->cnx, mxcb->empty_cursor);
      xcb_key_symbols_free(mxcb->keysymbols);
      xcb_disconnect(mxcb->cnx);
      free(mxcb);
    }
  return (NULL);
}


void	mlx__xcb_destroy(mlx_backend_hooks_param_t *param)
{
  mlx__xcb_t	*mxcb;

  mxcb = (mlx__xcb_t *)(param->backend);
  mlx__xcb_error(mxcb);
}


void *mlx__xcb_init(mlx_backend_hooks_param_t *param)
{
  mlx__xcb_t				*mxcb;
  xcb_screen_iterator_t			iter;
  int					i;
  xcb_generic_event_t			*event;
  xcb_pixmap_t				pixmap;
  xcb_font_t				font;

  if ( (mxcb = malloc(sizeof(*mxcb))) == NULL )
    return (NULL);
  
  /* Open the connection to the X server.
     Use the DISPLAY environment variable
  */
  mxcb->cnx = xcb_connect(NULL, &(mxcb->screen_preferred));
  if (xcb_connection_has_error(mxcb->cnx) > 0)
    return (mlx__xcb_error(mxcb));

  mxcb->setup = xcb_get_setup(mxcb->cnx);

  /* Get the screen #screen_nbr */
  iter = xcb_setup_roots_iterator(mxcb->setup);
  i = 0;
  while (iter.rem && i < mxcb->screen_preferred)
    xcb_screen_next(&iter);
  mxcb->screen = iter.data;
  param->size.width = mxcb->screen->width_in_pixels;
  param->size.height = mxcb->screen->height_in_pixels;
  
  mxcb->keysymbols = xcb_key_symbols_alloc(mxcb->cnx);

  // empty cursor
  pixmap = xcb_generate_id(mxcb->cnx);
  xcb_create_pixmap(mxcb->cnx, 1, pixmap, mxcb->screen->root, 1, 1);
  mxcb->empty_cursor = xcb_generate_id(mxcb->cnx);
  xcb_create_cursor(mxcb->cnx, mxcb->empty_cursor, pixmap, pixmap,
                     0, 0, 0,    // foreground color (noir)
                     0, 0, 0,    // background color (noir)  
                     0, 0);      // hotspot x, y
  xcb_free_pixmap(mxcb->cnx, pixmap);

  // arrow cursor
  font = xcb_generate_id(mxcb->cnx);
  xcb_open_font(mxcb->cnx, font, 6, "cursor");
  mxcb->default_cursor = xcb_generate_id(mxcb->cnx);
  xcb_create_glyph_cursor(mxcb->cnx, mxcb->default_cursor, font, font,
			  68, 69,
			  0, 0, 0,           // Noir
			  65535, 65535, 65535 // Blanc
			  );
  xcb_close_font(mxcb->cnx, font);
  
  xcb_flush(mxcb->cnx);
  while ((event = xcb_poll_for_event(mxcb->cnx)))
    free(event);
  
  return (mxcb);
}
