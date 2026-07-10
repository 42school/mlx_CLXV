
#define	_GNU_SOURCE

#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/mman.h>
#include	<linux/input-event-codes.h>

#include	"mlx_internal.h"

#include	<wayland-client.h>
#include	<xkbcommon/xkbcommon.h>

#include	"mlx__wayland_internal.h"


static mlx__wayland_win_t	*mlx__wayland_win_from_surface(mlx__wayland_t *wl,
							       struct wl_surface *surface)
{
  mlx__wayland_win_t	*win;

  win = wl->windows;
  while (win && win->surface != surface)
    win = win->next;
  return (win);
}


/* --- pointer --- */

static void	mlx__wayland_ptr_enter(void *data, struct wl_pointer *ptr,
				       uint32_t serial, struct wl_surface *surface,
				       wl_fixed_t x, wl_fixed_t y)
{
  mlx__wayland_t	*wl;
  mlx__wayland_win_t	*win;

  (void)ptr;
  wl = (mlx__wayland_t *)data;
  win = mlx__wayland_win_from_surface(wl, surface);
  wl->pointer_focus = win;
  wl->pointer_enter_serial = serial;
  if (win)
    {
      win->ptr_x = wl_fixed_to_int(x);
      win->ptr_y = wl_fixed_to_int(y);
    }
  mlx__wayland_cursor_set(wl, wl->cursor_visible);
}

static void	mlx__wayland_ptr_leave(void *data, struct wl_pointer *ptr,
				       uint32_t serial, struct wl_surface *surface)
{
  mlx__wayland_t	*wl;

  (void)ptr; (void)serial; (void)surface;
  wl = (mlx__wayland_t *)data;
  wl->pointer_focus = NULL;
}

static void	mlx__wayland_ptr_motion(void *data, struct wl_pointer *ptr,
					uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
  mlx__wayland_t	*wl;
  mlx__wayland_win_t	*win;

  (void)ptr; (void)time;
  wl = (mlx__wayland_t *)data;
  win = wl->pointer_focus;
  if (win == NULL)
    return ;
  win->ptr_x = wl_fixed_to_int(x);
  win->ptr_y = wl_fixed_to_int(y);
  mlx__wayland_queue_push(wl, win, MLX_WL_EVENT_MOTION, 0, win->ptr_x, win->ptr_y);
}

static void	mlx__wayland_ptr_button(void *data, struct wl_pointer *ptr,
					uint32_t serial, uint32_t time,
					uint32_t button, uint32_t state)
{
  mlx__wayland_t	*wl;
  mlx__wayland_win_t	*win;
  unsigned int		x11_button;

  (void)ptr; (void)serial; (void)time;
  wl = (mlx__wayland_t *)data;
  win = wl->pointer_focus;
  if (win == NULL)
    return ;
  if (button == BTN_LEFT)
    x11_button = 1;
  else if (button == BTN_MIDDLE)
    x11_button = 2;
  else if (button == BTN_RIGHT)
    x11_button = 3;
  else
    return ;
  mlx__wayland_queue_push(wl, win,
			   state == WL_POINTER_BUTTON_STATE_PRESSED ?
			   MLX_WL_EVENT_BUTTON_PRESS : MLX_WL_EVENT_BUTTON_RELEASE,
			   x11_button, win->ptr_x, win->ptr_y);
}

static void	mlx__wayland_ptr_axis(void *data, struct wl_pointer *ptr,
				      uint32_t time, uint32_t axis, wl_fixed_t value)
{
  mlx__wayland_t	*wl;
  mlx__wayland_win_t	*win;
  unsigned int		x11_button;

  (void)ptr; (void)time;
  if (axis != WL_POINTER_AXIS_VERTICAL_SCROLL)
    return ;
  wl = (mlx__wayland_t *)data;
  win = wl->pointer_focus;
  if (win == NULL)
    return ;
  /* X11 turns wheel notches into instantaneous button-4/5 clicks */
  x11_button = wl_fixed_to_double(value) < 0 ? 4 : 5;
  mlx__wayland_queue_push(wl, win, MLX_WL_EVENT_BUTTON_PRESS,
			   x11_button, win->ptr_x, win->ptr_y);
  mlx__wayland_queue_push(wl, win, MLX_WL_EVENT_BUTTON_RELEASE,
			   x11_button, win->ptr_x, win->ptr_y);
}

static const struct wl_pointer_listener	mlx__wayland_pointer_listener =
  {
    .enter = mlx__wayland_ptr_enter,
    .leave = mlx__wayland_ptr_leave,
    .motion = mlx__wayland_ptr_motion,
    .button = mlx__wayland_ptr_button,
    .axis = mlx__wayland_ptr_axis
  };


/* --- keyboard --- */

static void	mlx__wayland_kbd_keymap(void *data, struct wl_keyboard *kbd,
					uint32_t format, int fd, uint32_t size)
{
  mlx__wayland_t	*wl;
  char			*map_str;

  (void)kbd;
  wl = (mlx__wayland_t *)data;
  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
      close(fd);
      return ;
    }
  map_str = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (map_str == MAP_FAILED)
    return ;
  if (wl->xkb_state)
    xkb_state_unref(wl->xkb_state);
  if (wl->xkb_keymap)
    xkb_keymap_unref(wl->xkb_keymap);
  wl->xkb_keymap = xkb_keymap_new_from_string(wl->xkb_context, map_str,
					       XKB_KEYMAP_FORMAT_TEXT_V1,
					       XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(map_str, size);
  wl->xkb_state = wl->xkb_keymap ? xkb_state_new(wl->xkb_keymap) : NULL;
}

static void	mlx__wayland_kbd_enter(void *data, struct wl_keyboard *kbd,
				       uint32_t serial, struct wl_surface *surface,
				       struct wl_array *keys)
{
  mlx__wayland_t	*wl;

  (void)kbd; (void)serial; (void)keys;
  wl = (mlx__wayland_t *)data;
  wl->keyboard_focus = mlx__wayland_win_from_surface(wl, surface);
}

static void	mlx__wayland_kbd_leave(void *data, struct wl_keyboard *kbd,
				       uint32_t serial, struct wl_surface *surface)
{
  mlx__wayland_t	*wl;

  (void)kbd; (void)serial; (void)surface;
  wl = (mlx__wayland_t *)data;
  wl->keyboard_focus = NULL;
}

static void	mlx__wayland_kbd_key(void *data, struct wl_keyboard *kbd,
				     uint32_t serial, uint32_t time,
				     uint32_t key, uint32_t state)
{
  mlx__wayland_t	*wl;
  xkb_keysym_t		sym;

  (void)kbd; (void)serial; (void)time;
  wl = (mlx__wayland_t *)data;
  if (wl->xkb_state == NULL || wl->keyboard_focus == NULL)
    return ;
  /* evdev keycode -> xkb keycode offset, xkb keysym values match X11 keysyms */
  sym = xkb_state_key_get_one_sym(wl->xkb_state, key + 8);
  xkb_state_update_key(wl->xkb_state, key + 8,
		       state == WL_KEYBOARD_KEY_STATE_PRESSED ?
		       XKB_KEY_DOWN : XKB_KEY_UP);
  mlx__wayland_queue_push(wl, wl->keyboard_focus,
			   state == WL_KEYBOARD_KEY_STATE_PRESSED ?
			   MLX_WL_EVENT_KEY_PRESS : MLX_WL_EVENT_KEY_RELEASE,
			   (unsigned int)sym, 0, 0);
}

static void	mlx__wayland_kbd_modifiers(void *data, struct wl_keyboard *kbd,
					   uint32_t serial, uint32_t mods_depressed,
					   uint32_t mods_latched, uint32_t mods_locked,
					   uint32_t group)
{
  mlx__wayland_t	*wl;

  (void)kbd; (void)serial;
  wl = (mlx__wayland_t *)data;
  if (wl->xkb_state == NULL)
    return ;
  xkb_state_update_mask(wl->xkb_state, mods_depressed, mods_latched,
			 mods_locked, 0, 0, group);
}

static const struct wl_keyboard_listener	mlx__wayland_keyboard_listener =
  {
    .keymap = mlx__wayland_kbd_keymap,
    .enter = mlx__wayland_kbd_enter,
    .leave = mlx__wayland_kbd_leave,
    .key = mlx__wayland_kbd_key,
    .modifiers = mlx__wayland_kbd_modifiers
  };


/* --- seat --- */

static void	mlx__wayland_seat_capabilities(void *data, struct wl_seat *seat,
						uint32_t caps)
{
  mlx__wayland_t	*wl;

  (void)seat;
  wl = (mlx__wayland_t *)data;
  if ((caps & WL_SEAT_CAPABILITY_POINTER) && wl->pointer == NULL)
    {
      wl->pointer = wl_seat_get_pointer(wl->seat);
      wl_pointer_add_listener(wl->pointer, &mlx__wayland_pointer_listener, wl);
    }
  else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && wl->pointer)
    {
      wl_pointer_destroy(wl->pointer);
      wl->pointer = NULL;
    }
  if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && wl->keyboard == NULL)
    {
      wl->keyboard = wl_seat_get_keyboard(wl->seat);
      wl_keyboard_add_listener(wl->keyboard, &mlx__wayland_keyboard_listener, wl);
    }
  else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && wl->keyboard)
    {
      wl_keyboard_destroy(wl->keyboard);
      wl->keyboard = NULL;
    }
}

static const struct wl_seat_listener	mlx__wayland_seat_listener =
  {
    .capabilities = mlx__wayland_seat_capabilities
  };


int	mlx__wayland_seat_init(mlx__wayland_t *wl)
{
  wl_seat_add_listener(wl->seat, &mlx__wayland_seat_listener, wl);
  wl_display_roundtrip(wl->display);   /* get capabilities, bind pointer/keyboard */
  wl_display_roundtrip(wl->display);   /* get the initial keymap */
  return (0);
}

void	mlx__wayland_seat_destroy(mlx__wayland_t *wl)
{
  if (wl->keyboard)
    wl_keyboard_destroy(wl->keyboard);
  if (wl->pointer)
    wl_pointer_destroy(wl->pointer);
  if (wl->seat)
    wl_seat_destroy(wl->seat);
  if (wl->xkb_state)
    xkb_state_unref(wl->xkb_state);
  if (wl->xkb_keymap)
    xkb_keymap_unref(wl->xkb_keymap);
  if (wl->xkb_context)
    xkb_context_unref(wl->xkb_context);
}
