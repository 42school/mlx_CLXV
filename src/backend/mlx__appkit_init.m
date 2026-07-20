
#include	<stdlib.h>
#include	<strings.h>

#include	"mlx_internal.h"
#include	"mlx__appkit_internal.h"

#import	<Cocoa/Cocoa.h>


void	*mlx__appkit_init(mlx_backend_hooks_param_t *param)
{
  mlx__appkit_t	*ak;
  NSScreen	*screen;
  NSRect	frame;

  if ((ak = malloc(sizeof(*ak))) == NULL)
    return (NULL);
  bzero(ak, sizeof(*ak));
  ak->cursor_visible = 1;
  ak->key_autorepeat = 1;

  @autoreleasepool
    {
      /* a regular activation policy is needed for a window launched
	 from a plain command-line binary to get a Dock icon, become
	 the frontmost app, and receive keyboard focus at all */
      [NSApplication sharedApplication];
      [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
      [NSApp finishLaunching];
      [NSApp activateIgnoringOtherApps:YES];

      screen = [NSScreen mainScreen];
      if (screen)
	{
	  frame = [screen frame];
	  ak->screen_width = (unsigned int)frame.size.width;
	  ak->screen_height = (unsigned int)frame.size.height;
	}
    }
  if (ak->screen_width == 0 || ak->screen_height == 0)
    {
      ak->screen_width = 1920;
      ak->screen_height = 1080;
    }
  param->size.width = ak->screen_width;
  param->size.height = ak->screen_height;
  return (ak);
}

void	mlx__appkit_destroy(mlx_backend_hooks_param_t *param)
{
  mlx__appkit_t	*ak;

  ak = (mlx__appkit_t *)(param->backend);
  if (ak)
    free(ak);
}
