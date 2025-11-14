# MinilibX Makefile
# Just run `make`

NAME=libmlx.so
SRC=src/mlx_init.c src/mlx_window.c src/mlx_image.c src/mlx_do_sync.c src/mlx_loop.c \
	src/mlx_key_hook.c src/mlx_mouse_hook.c src/mlx_expose_hook.c src/mlx_loop_hook.c \
	src/mlx_hook.c src/mlx_be_gpu_hooks.c src/mlx_xpm.c src/mlx_png.c src/mlx_string_put.c \
	src/mlx_be_extra.c
SRC_XCB=src/backend/mlx__xcb_init.c src/backend/mlx__xcb_window.c src/backend/mlx__xcb_flush.c \
	src/backend/mlx__xcb_anti_resize_win.c src/backend/mlx__xcb_loop.c src/backend/mlx__xcb_event.c \
	src/backend/mlx__xcb_hook.c src/backend/mlx__xcb_extra.c
SRC_VULKAN=src/gpu/mlx___vulkan_init.c src/gpu/mlx___vulkan_window.c src/gpu/mlx___vulkan_draw.c \
	src/gpu/mlx___vulkan_image.c
SRC+=$(SRC_XCB) $(SRC_VULKAN)

OBJ=$(SRC:.c=.o)

#VK_DEBUG=-DVK_DEBUG_LAYER
VK_DEBUG=

INCLUDES=-I./src
CFLAGS+= $(INCLUDES) $(VK_DEBUG) -fPIC -Wall -O3
LDFLAGS+=

CC=clang

LIBS= -lxcb -lxcb-keysyms -lvulkan -lz -lbsd


all: config $(NAME) pypkg

config: configure.sh
	./configure.sh

$(NAME): $(OBJ)
	@echo "Building library..."
	$(CC) -shared -o $(NAME) $(CFLAGS) $(LDFLAGS) $(OBJ) $(LIBS)

pypkg: $(NAME) pybuild.sh
	@echo "Building Python package"
	cp $(NAME) python/src/mlx/
	cp src/mlx.h man/man3/* python/src/mlx/docs/
	./pybuild.sh
	cp python/dist/mlx*.whl .

clean:
	rm -rf $(NAME) $(OBJ) *~ src/*~ src/backend/*~ src/gpu/*~ venv python/src/mlx/docs/* python/src/mlx/$(NAME) python/dist test/*~ mlx*.whl python/*~ python/src/mlx.egg-info

re: clean all
