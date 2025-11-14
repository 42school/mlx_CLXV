/*
** mlx.h for MinilibX in 
** 
** Made by Charlie Root
** Login   <ol@42.fr>
** 
** Started on  Mon Jul 31 16:37:50 2000 Olivier Crouzet
** Last update Tue Jun 25 16:23:28 2025 Olivier Crouzet
*/

/*
**   MinilibX -  Please report bugs
*/


/* mlx_CLXV version 2.2 */

/*
**
** This library is a simple framework to help 42 students
** create simple graphical apps.
** It only provides the minimum functions, it's students' job
** to create the missing pieces for their own project :)
**
** Current XCB-Vulkan requirements for Linux:
**          libxcb, libxcb-keysyms, libvulkan,
**          libz, libbsd
** You also need glslc to re-compile shaders if needed.
** At 42, on current Ubuntu 22.04 dump in cluster, you need to get
**  libxcb-keysyms source for the include file and compile the .a library.
**
** The MinilibX can load XPM and PNG images.
** Please note that both image loaders are incomplete, some
** image may not load. Also, image loaders only work for little endian hosts.
**
** Historically, the alpha byte did represent transparency
** instead of opacity. It's not the case anymore. MLX matches GPUs standards.
**
** MLX_CLXV API changes:
**  - mlx_get_data_addr now provides the image format instead of the
        endian, and returns an 'unsigned char' pointer.
**  - 'unsigned int' replace 'int' in many calls.
**  - mlx_get_color_value() is now deprecated.
**  - adding mlx_loop_exit().
**
** With recent X11 implementation and default configuration, the Expose event is only
**  received once at the program launch. This is often due to X server saving the
**  content of the window.
** With Wayland, there is no such thing like Expose event, and the compositor saves
**  the window's content.
**
*/


#ifndef MLX_H

#define	MLX_H


/*
**  mlx_init() is needed before everything else.
**  mlx_init() returns 'void *0' in case of failure.
**  mlx_release() returns 0 on success.
*/
void	*mlx_init();
int	mlx_release(void *mlx_ptr);


/*
** Window actions
*/
void	*mlx_new_window(void *mlx_ptr, unsigned int width,
			unsigned int height, const char *title);
int	mlx_clear_window(void *mlx_ptr, void *win_ptr);
int	mlx_pixel_put(void *mlx_ptr, void *win_ptr,
		      unsigned int x, unsigned int y, unsigned int color);
int	mlx_destroy_window(void *mlx_ptr, void *win_ptr);
/*
**  mlx_new_window() returns 'void *0' if failed.
**  Other functions return 0 on success.
**  Origin for x & y is top left corner of the window, y down is positive.
**  x and y must fit into the size of the window, values are not controled
**  Color byte order is B8G8R8A8, which could be 0xAARRGGBB or 0xBBGGRRAA
**   depending on local endianess.
*/


/*
** Images
*/
void	*mlx_new_image(void *mlx_ptr, unsigned int width, unsigned int height);
unsigned char	*mlx_get_data_addr(void *img_ptr, unsigned int *bits_per_pixel,
				   unsigned int *size_line,
				   unsigned int *format);
int	mlx_put_image_to_window(void *mlx_ptr, void *win_ptr, void *img_ptr,
				int x, int y);
int	mlx_destroy_image(void *mlx_ptr, void *img_ptr);
/*
** mlx_new_image() returns 'void *0' in case of failure.
** mlx_get_data_addr() returns a pointer to a height * size_line bytes buffer
**   that holds the pixel values.
** Other functions return 0 on success.
** 'format' can be: 0 = B8G8R8A8; 1 = A8R8G8B8; (byte order).
** Carefully consider the format, it can be reversed in some cases, like a remote graphic server
*/


/*
** deprecated function - format of image allows conversion on student's side
** unsigned int	mlx_get_color_value(void *mlx_ptr, int color);
**
*/


/*
** main loop & dealing with events
*/
typedef int (*mlx_mouse_callback)(unsigned int, unsigned int, unsigned int, void*);
typedef int (*mlx_key_callback)(unsigned int, void *);
typedef int (*mlx_expose_callback)(void *);
typedef int (*mlx_loop_callback)(void *);
typedef int (*mlx_hook_callback)(void *);

int	mlx_loop(void *mlx_ptr);
int	mlx_loop_exit(void *mlx_ptr);
int	mlx_mouse_hook(void *win_ptr, mlx_mouse_callback funct_ptr, void *param);
int	mlx_key_hook(void *win_ptr, mlx_key_callback funct_ptr, void *param);
int	mlx_expose_hook(void *win_ptr, mlx_expose_callback funct_ptr, void *param);
int	mlx_loop_hook(void *mlx_ptr, mlx_loop_callback funct_ptr, void *param);
/*
**  Functions return 0 on success.
**  Key event is triggered on KeyRelease, not KeyPressed.
**  Mouse event is triggered on clic.
**
**  hook functions are called as follow:
**    expose_hook(void *param);
**    key_hook(unsigned int keycode, void *param);
**    mouse_hook(unsigned int button, unsigned int x, unsigned int y,
**               void *param);
**    loop_hook(void *param);
*/

/*
**  Generic hook system for all events, and minilibX functions that
**    can be hooked. Some macro and defines from X11/X.h are needed here.
**  Warning: you may need to cast your function pointer for key and mouse events
**   as there will be extra parameters.
*/
int	mlx_hook(void *win_ptr, unsigned int x_event, unsigned int x_mask,
                 mlx_hook_callback funct_ptr, void *param);


/*
**  Convenience functions
**  mlx_string_put() display may vary in size between OS and between
**    mlx implementations
**  mlx_string_put() returns 0 on success.
**  Other functions return an image (like mlx_new_image()) or 'void *0'.
**
*/
int	mlx_string_put(void *mlx_ptr, void *win_ptr,
		       unsigned int x, unsigned int y,
		       unsigned int color, char *string);
void	*mlx_xpm_to_image(void *mlx_ptr, const char **xpm_data,
			  unsigned int *width, unsigned int *height);
void	*mlx_xpm_file_to_image(void *mlx_ptr, const char *filename,
			       unsigned int *width, unsigned int *height);
void    *mlx_png_file_to_image(void *mlx_ptr, const char *filename,
			       unsigned int *width, unsigned int *height);



/*
** Convenience functions
** All functions return 0 on success.
*/
int     mlx_mouse_hide(void *mlx_ptr);
int     mlx_mouse_show(void *mlx_ptr);
int     mlx_mouse_move(void *win_ptr, int x, int y);
int     mlx_mouse_get_pos(void *win_ptr, int *x, int *y);
int	mlx_do_key_autorepeatoff(void *mlx_ptr);
int	mlx_do_key_autorepeaton(void *mlx_ptr);
int	mlx_get_screen_size(void *mlx_ptr,
			    unsigned int *width, unsigned int *height);


/*
** Flush & Sync
*/
int	mlx_do_sync(void *mlx_ptr);
#define MLX_SYNC_IMAGE_WRITABLE		1
#define MLX_SYNC_WIN_FLUSH		2
#define MLX_SYNC_WIN_COMPLETED		3
int	mlx_sync(void *mlx_ptr, int cmd, void *param);
/*
** Functions return 0 on success.
** mlx_do_sync() will *flush* (not sync) all requests and wait for completion.
** Note: mlx_loop() always flush requests.
** mlx_sync() 'cmd' commands are:
**   - 'image_writable' returns when image data can be written again.
**   - 'win_flush' returns when all pending requests are sent to server.
**   - 'win_completed' returns after flush and completion.
**   'param' is image pointer or window pointer, according to the command.
** mlx_do_sync() equals 'win_flush' for all windows.
**
*/


#endif /* MLX_H */
