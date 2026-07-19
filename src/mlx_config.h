

#ifndef MLX_CONFIG_H
#define MLX_CONFIG_H


#define	MLX_BACKEND_XCB		0
#define	MLX_BACKEND_WAYLAND	1
#define	MLX_BACKEND_APPKIT	2
/* possible other backend : MS-Windows ... */

#define	MLX_GPU_VULKAN	0
/* possibles other GPUs : OpenGL, Metal ... */



/* overridable with -D in Makefile : make BACKEND=wayland / BACKEND=appkit */

#ifndef MLX_BACKEND
 #define	MLX_BACKEND	MLX_BACKEND_XCB
#endif

#ifndef	MLX_GPU
 #define	MLX_GPU		MLX_GPU_VULKAN
#endif

#endif
