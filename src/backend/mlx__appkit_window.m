
#include	<stdlib.h>
#include	<strings.h>

#include	"mlx_internal.h"
#include	"mlx__appkit_internal.h"

#import	<Cocoa/Cocoa.h>
#import	<QuartzCore/QuartzCore.h>


/* --- custom content view: forwards input events to the queue --- */

@interface	MLXView : NSView
@property (nonatomic, assign) mlx__appkit_win_t *mlxWin;
@end

@implementation MLXView

- (BOOL)acceptsFirstResponder
{
  return (YES);
}

- (BOOL)isFlipped
{
  /* mlx's origin is top-left, y down - NSView's default is bottom-left */
  return (YES);
}

- (void)mouseDown:(NSEvent *)event { mlx__appkit_on_button(self.mlxWin, event, MLX_AK_EVENT_BUTTON_PRESS); }
- (void)mouseUp:(NSEvent *)event { mlx__appkit_on_button(self.mlxWin, event, MLX_AK_EVENT_BUTTON_RELEASE); }
- (void)rightMouseDown:(NSEvent *)event { mlx__appkit_on_button(self.mlxWin, event, MLX_AK_EVENT_BUTTON_PRESS); }
- (void)rightMouseUp:(NSEvent *)event { mlx__appkit_on_button(self.mlxWin, event, MLX_AK_EVENT_BUTTON_RELEASE); }
- (void)otherMouseDown:(NSEvent *)event { mlx__appkit_on_button(self.mlxWin, event, MLX_AK_EVENT_BUTTON_PRESS); }
- (void)otherMouseUp:(NSEvent *)event { mlx__appkit_on_button(self.mlxWin, event, MLX_AK_EVENT_BUTTON_RELEASE); }

- (void)mouseMoved:(NSEvent *)event { mlx__appkit_on_motion(self.mlxWin, event); }
- (void)mouseDragged:(NSEvent *)event { mlx__appkit_on_motion(self.mlxWin, event); }
- (void)rightMouseDragged:(NSEvent *)event { mlx__appkit_on_motion(self.mlxWin, event); }
- (void)otherMouseDragged:(NSEvent *)event { mlx__appkit_on_motion(self.mlxWin, event); }

- (void)scrollWheel:(NSEvent *)event { mlx__appkit_on_scroll(self.mlxWin, event); }

- (void)keyDown:(NSEvent *)event { mlx__appkit_on_key(self.mlxWin, event, MLX_AK_EVENT_KEY_PRESS); }
- (void)keyUp:(NSEvent *)event { mlx__appkit_on_key(self.mlxWin, event, MLX_AK_EVENT_KEY_RELEASE); }

@end


/* --- window delegate: only cares about the close button --- */

@interface	MLXWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) mlx__appkit_win_t *mlxWin;
@end

@implementation MLXWindowDelegate

- (BOOL)windowShouldClose:(NSWindow *)sender
{
  (void)sender;
  /* mirrors XCB/Wayland: WM_DELETE_WINDOW-equivalent does nothing on
     its own, the student must catch event 33 and act on it themselves */
  mlx__appkit_queue_push(self.mlxWin->ak, self.mlxWin, MLX_AK_EVENT_CLOSE, 0, 0, 0);
  return (NO);
}

@end


static void	*mlx__appkit_window_error(mlx__appkit_win_t *win)
{
  if (win)
    {
      if (win->window)
	{
	  win->window.delegate = nil;
	  [win->window close];
	  [win->window release];
	}
      if (win->delegate)
	[win->delegate release];
      if (win->view)
	[win->view release];
      free(win);
    }
  return (NULL);
}

void	mlx__appkit_window_destroy(mlx_backend_hooks_param_t *param)
{
  mlx__appkit_t		*ak;
  mlx__appkit_win_t	*win;

  ak = (mlx__appkit_t *)(param->backend);
  win = (mlx__appkit_win_t *)(param->backend_win);
  if (win)
    mlx__appkit_queue_purge_win(ak, win);
  @autoreleasepool
    {
      mlx__appkit_window_error(win);
    }
}

void	*mlx__appkit_window(mlx_backend_hooks_param_t *param)
{
  mlx__appkit_win_t	*win;
  NSRect		rect;
  NSWindowStyleMask	style;

  if ((win = malloc(sizeof(*win))) == NULL)
    return (NULL);
  bzero(win, sizeof(*win));
  win->ak = (mlx__appkit_t *)(param->backend);
  win->width = param->size.width;
  win->height = param->size.height;

  @autoreleasepool
    {
      rect = NSMakeRect(0, 0, win->width, win->height);
      style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
	NSWindowStyleMaskMiniaturizable;
      win->window = [[NSWindow alloc] initWithContentRect:rect styleMask:style
						    backing:NSBackingStoreBuffered
						      defer:NO];
      if (win->window == nil)
	return (mlx__appkit_window_error(win));
      win->window.releasedWhenClosed = NO;
      /* anti-resize: mirrors mlx__xcb_anti_resize_win() / the fixed
	 size locked via xdg_toplevel_set_min/max_size on Wayland */
      win->window.contentMinSize = rect.size;
      win->window.contentMaxSize = rect.size;
      if (param->title)
	win->window.title = [NSString stringWithUTF8String:param->title];
      [win->window center];
      win->window.acceptsMouseMovedEvents = YES;

      win->view = [[MLXView alloc] initWithFrame:rect];
      win->view.mlxWin = win;
      win->view.wantsLayer = YES;
      win->metal_layer = [CAMetalLayer layer];
      win->view.layer = win->metal_layer;
      win->window.contentView = win->view;
      [win->window makeFirstResponder:win->view];

      win->delegate = [[MLXWindowDelegate alloc] init];
      win->delegate.mlxWin = win;
      win->window.delegate = win->delegate;

      [win->window makeKeyAndOrderFront:nil];
      [NSApp activateIgnoringOtherApps:YES];

      /* Wayland has no Expose event and the compositor keeps the
	 window's content around itself; treat AppKit the same way and
	 only fire this once, right after the window is first shown */
      mlx__appkit_queue_push(win->ak, win, MLX_AK_EVENT_EXPOSE, 0, 0, 0);
      win->configured = 1;
    }
  return (win);
}
