# MinilibX Makefile
# Just run `make`
# To build the Wayland backend instead of XCB: `make BACKEND=wayland`

BACKEND?=xcb

NAME=libmlx.so
SRC=src/mlx_init.c src/mlx_window.c src/mlx_image.c src/mlx_do_sync.c src/mlx_loop.c \
	src/mlx_key_hook.c src/mlx_mouse_hook.c src/mlx_expose_hook.c src/mlx_loop_hook.c \
	src/mlx_hook.c src/mlx_be_gpu_hooks.c src/mlx_xpm.c src/mlx_png.c src/mlx_string_put.c \
	src/mlx_be_extra.c
SRC_XCB=src/backend/mlx__xcb_init.c src/backend/mlx__xcb_window.c src/backend/mlx__xcb_flush.c \
	src/backend/mlx__xcb_anti_resize_win.c src/backend/mlx__xcb_loop.c src/backend/mlx__xcb_event.c \
	src/backend/mlx__xcb_hook.c src/backend/mlx__xcb_extra.c
SRC_WAYLAND=src/backend/mlx__wayland_init.c src/backend/mlx__wayland_window.c \
	src/backend/mlx__wayland_flush.c src/backend/mlx__wayland_seat.c \
	src/backend/mlx__wayland_cursor.c src/backend/mlx__wayland_hook.c \
	src/backend/mlx__wayland_event.c src/backend/mlx__wayland_extra.c \
	src/backend/mlx__wayland_util.c \
	src/backend/mlx__wayland_xdg_shell_protocol.c
SRC_VULKAN=src/gpu/mlx___vulkan_init.c src/gpu/mlx___vulkan_window.c src/gpu/mlx___vulkan_draw.c \
	src/gpu/mlx___vulkan_image.c

ifeq ($(BACKEND),wayland)
SRC+=$(SRC_WAYLAND) $(SRC_VULKAN)
CFLAGS+=-DMLX_BACKEND=MLX_BACKEND_WAYLAND
LIBS_BACKEND=-lwayland-client -lwayland-cursor -lxkbcommon
WAYLAND_PROTOCOLS_DIR:=$(shell pkg-config --variable=pkgdatadir wayland-protocols)
XDG_SHELL_XML:=$(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml
# pointer-warp-v1 is a staging (not yet stable) protocol used for
# mlx_mouse_move(); only wired in when the installed wayland-protocols
# package has it and the compositor may or may not support it at
# runtime either way - mlx__wayland_extra.c falls back gracefully
POINTER_WARP_XML:=$(WAYLAND_PROTOCOLS_DIR)/staging/pointer-warp/pointer-warp-v1.xml
ifneq ($(wildcard $(POINTER_WARP_XML)),)
SRC+=src/backend/mlx__wayland_pointer_warp_protocol.c
CFLAGS+=-DMLX_WAYLAND_HAVE_POINTER_WARP
endif
else
SRC+=$(SRC_XCB) $(SRC_VULKAN)
CFLAGS+=-DMLX_BACKEND=MLX_BACKEND_XCB
LIBS_BACKEND=-lxcb -lxcb-keysyms -lbsd
endif

OBJ=$(SRC:.c=.o)

ifeq ($(BACKEND),wayland)
# every wayland source transitively includes the generated xdg-shell
# client header, make sure it exists before any of them gets compiled
$(OBJ): src/backend/mlx__wayland_xdg_shell_protocol.h
ifneq ($(wildcard $(POINTER_WARP_XML)),)
$(OBJ): src/backend/mlx__wayland_pointer_warp_protocol.h
endif
endif

#VK_DEBUG=-DVK_DEBUG_LAYER
VK_DEBUG=

INCLUDES=-I./src
CFLAGS+= $(INCLUDES) $(VK_DEBUG) -fPIC -Wall -O3
LDFLAGS+=

CC=clang

LIBS= $(LIBS_BACKEND) -lvulkan -lz


all: config $(NAME) pypkg

config: configure.sh
	BACKEND=$(BACKEND) ./configure.sh

# xdg-shell is the only Wayland protocol extension needed (window
# management); its client bindings are generated at build time from
# the wayland-protocols package rather than committed to the repo.
src/backend/mlx__wayland_xdg_shell_protocol.h: $(XDG_SHELL_XML)
	wayland-scanner client-header $< $@

src/backend/mlx__wayland_xdg_shell_protocol.c: $(XDG_SHELL_XML) src/backend/mlx__wayland_xdg_shell_protocol.h
	wayland-scanner private-code $< $@

src/backend/mlx__wayland_pointer_warp_protocol.h: $(POINTER_WARP_XML)
	wayland-scanner client-header $< $@

src/backend/mlx__wayland_pointer_warp_protocol.c: $(POINTER_WARP_XML) src/backend/mlx__wayland_pointer_warp_protocol.h
	wayland-scanner private-code $< $@

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
	rm -rf $(NAME) $(OBJ) *~ src/*~ src/backend/*~ src/gpu/*~ venv python/src/mlx/docs/* python/src/mlx/$(NAME) python/dist test/*~ mlx*.whl python/*~ python/src/mlx.egg-info src/backend/mlx__wayland_xdg_shell_protocol.h src/backend/mlx__wayland_xdg_shell_protocol.c src/backend/mlx__wayland_pointer_warp_protocol.h src/backend/mlx__wayland_pointer_warp_protocol.c

re: clean all
