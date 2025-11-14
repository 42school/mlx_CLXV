
/* mlx__xcb_internal.h */

#ifndef MLX__XCB_INTERNAL_H
#define MLX__XCB_INTERNAL_H

#define	MIN(X,Y)	((X)<=(Y)?(X):(Y))

#define	MLX_XCB_MAX_EVENT	36

#define	MLX_XCB_EXT_FIXES	(1<<0)
#define	MLX_XCB_EXT_XKB		(1<<1)


typedef struct 			mlx__xcb_s
{
  xcb_connection_t		*cnx;
  int				screen_preferred;
  xcb_screen_t			*screen;
  const struct xcb_setup_t	*setup;
  xcb_key_symbols_t 		*keysymbols;
  xcb_cursor_t			default_cursor;
  xcb_cursor_t			empty_cursor;
  unsigned int			ext;
} mlx__xcb_t;


typedef struct		mlx__xcb_win_s
{
  mlx__xcb_t		*xcb;   /* needed for key / mouse hooks */
  xcb_window_t		win_id;
  unsigned int		width;
  unsigned int		height;
  int			(*hook[MLX_XCB_MAX_EVENT])();
  void			*hook_param[MLX_XCB_MAX_EVENT];
  uint32_t		mask;
} mlx__xcb_win_t;




/* Internal prototypes */

void mlx__xcb_anti_resize_win(mlx__xcb_t *xcb, mlx__xcb_win_t *win);
void mlx__xcb_new_window_protocols(mlx__xcb_t *xcb, mlx__xcb_win_t *win);


#endif /* MLX__XCB_INTERNAL_H */
