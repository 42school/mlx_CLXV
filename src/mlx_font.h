
/* mlx_font.h - shared declaration for the font atlas baked into
   mlx_font.c, so it can be linked from more than one translation unit
   (mlx_string_put.c, and the Wayland backend's fake title bar) without
   duplicating the ~90KB pixel table in each */

#ifndef MLX_FONT_H
#define MLX_FONT_H

typedef struct			mlx_font_atlas_s
{
  unsigned int	width;
  unsigned int	height;
  unsigned int	bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char	pixel_data[1140 * 20 * 4 + 1];
}				mlx_font_atlas_t;

extern const mlx_font_atlas_t	font_atlas;

#endif /* MLX_FONT_H */
