
/* mlx_xcb_wmhints.h */

#ifndef MLX_XCB_WMHINTS_H
#define MLX_XCB_WMHINTS_H


// for anti resize win
struct		WMSizeHints
{
  uint32_t	flags;
  int32_t	x;
  int32_t	y;
  int32_t	width;
  int32_t	height;
  int32_t	min_width;
  int32_t	min_height;
  int32_t	max_width;
  int32_t	max_height;
  int32_t	width_inc;
  int32_t	height_inc;
  int32_t	min_aspect_num;
  int32_t	min_aspect_den;
  int32_t	max_aspect_num;
  int32_t	max_aspect_den;
  int32_t	base_width;
  int32_t	base_height;
  uint32_t	win_gravity;
};
enum WMSizeHintsFlag
{
  WM_SIZE_HINT_US_POSITION   = 1U << 0,
  WM_SIZE_HINT_US_SIZE       = 1U << 1,
  WM_SIZE_HINT_P_POSITION    = 1U << 2,
  WM_SIZE_HINT_P_SIZE        = 1U << 3,
  WM_SIZE_HINT_P_MIN_SIZE    = 1U << 4,
  WM_SIZE_HINT_P_MAX_SIZE    = 1U << 5,
  WM_SIZE_HINT_P_RESIZE_INC  = 1U << 6,
  WM_SIZE_HINT_P_ASPECT      = 1U << 7,
  WM_SIZE_HINT_BASE_SIZE     = 1U << 8,
  WM_SIZE_HINT_P_WIN_GRAVITY = 1U << 9
};


#endif   /* MLX_XCB_WMHINTS_H */
