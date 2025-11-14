
/* mlx__xcb_event.h */

#ifndef MLX__XCB_EVENT_H
#define MLX__XCB_EVENT_H


/* events tab with window pos */
typedef struct	mlx__xcb_event_s
{
  uint8_t	event;
  size_t	wpos;   /* 0 means no window attached to event so call all windows */
  void		(*handle)(mlx__xcb_t *, mlx__xcb_win_t *, xcb_generic_event_t *);
} mlx__xcb_event_t;

void mlx__xcb_event_generic();
void mlx__xcb_event_key();
void mlx__xcb_event_mouse();
void mlx__xcb_event_motion();

mlx__xcb_event_t mlx__xcb_event_tab[] =
  {
    { 0, 0, mlx__xcb_event_generic },
    { 1, 0, mlx__xcb_event_generic },
    { XCB_KEY_PRESS, offsetof(xcb_key_press_event_t, event), mlx__xcb_event_key },
    { XCB_KEY_RELEASE, offsetof(xcb_key_release_event_t, event), mlx__xcb_event_key },
    { XCB_BUTTON_PRESS, offsetof(xcb_button_press_event_t, event), mlx__xcb_event_mouse },
    { XCB_BUTTON_RELEASE, offsetof(xcb_button_release_event_t, event), mlx__xcb_event_mouse },
    { XCB_MOTION_NOTIFY, offsetof(xcb_motion_notify_event_t, event), mlx__xcb_event_motion },
    { XCB_ENTER_NOTIFY, offsetof(xcb_enter_notify_event_t, event), mlx__xcb_event_generic },
    { XCB_LEAVE_NOTIFY, offsetof(xcb_leave_notify_event_t, event), mlx__xcb_event_generic },
    { XCB_FOCUS_IN, offsetof(xcb_focus_in_event_t, event), mlx__xcb_event_generic },
    { XCB_FOCUS_OUT, offsetof(xcb_focus_out_event_t, event), mlx__xcb_event_generic },
    { XCB_KEYMAP_NOTIFY, 0, mlx__xcb_event_generic },
    { XCB_EXPOSE, offsetof(xcb_expose_event_t, window), mlx__xcb_event_generic },
    { XCB_GRAPHICS_EXPOSURE, 0, mlx__xcb_event_generic },
    { XCB_NO_EXPOSURE, 0, mlx__xcb_event_generic },
    { XCB_VISIBILITY_NOTIFY, offsetof(xcb_visibility_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_CREATE_NOTIFY, offsetof(xcb_create_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_DESTROY_NOTIFY, offsetof(xcb_destroy_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_UNMAP_NOTIFY, offsetof(xcb_unmap_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_MAP_NOTIFY, offsetof(xcb_map_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_MAP_REQUEST, offsetof(xcb_map_request_event_t, window), mlx__xcb_event_generic },
    { XCB_REPARENT_NOTIFY, offsetof(xcb_reparent_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_CONFIGURE_NOTIFY, offsetof(xcb_configure_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_CONFIGURE_REQUEST, offsetof(xcb_configure_request_event_t, window), mlx__xcb_event_generic },
    { XCB_GRAVITY_NOTIFY, offsetof(xcb_gravity_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_RESIZE_REQUEST, offsetof(xcb_resize_request_event_t, window), mlx__xcb_event_generic },
    { XCB_CIRCULATE_NOTIFY, offsetof(xcb_circulate_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_CIRCULATE_REQUEST, offsetof(xcb_circulate_request_event_t, window), mlx__xcb_event_generic },
    { XCB_PROPERTY_NOTIFY, offsetof(xcb_property_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_SELECTION_CLEAR, offsetof(xcb_selection_clear_event_t, owner), mlx__xcb_event_generic },
    { XCB_SELECTION_REQUEST, offsetof(xcb_selection_request_event_t, owner), mlx__xcb_event_generic },
    { XCB_SELECTION_NOTIFY, offsetof(xcb_selection_notify_event_t, requestor), mlx__xcb_event_generic },
    { XCB_COLORMAP_NOTIFY, offsetof(xcb_colormap_notify_event_t, window), mlx__xcb_event_generic },
    { XCB_CLIENT_MESSAGE, offsetof(xcb_client_message_event_t, window), mlx__xcb_event_generic },
    { XCB_MAPPING_NOTIFY, 0, mlx__xcb_event_generic },
    { XCB_GE_GENERIC, 0, mlx__xcb_event_generic }
  };



#endif /* MLX__XCB_EVENT_H */
