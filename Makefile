# MinilibX Makefile
# Just run `make`
# To build the Wayland backend instead of XCB: `make BACKEND=wayland`
#
# If a Wayland build dependency is missing (e.g. wayland-protocols), run
# `make BACKEND=wayland config` first to see configure.sh's diagnostic:
# Make resolves the whole dependency graph before running any recipe, so
# a missing package surfaces as Make's own opaque "No rule to make target"
# error if you go straight to `make BACKEND=wayland` without configuring
# first - same reason a plain `./configure && make` requires configure to
# run to completion before the build starts.

# set explicitly rather than relying on "first rule in the file" - the
# Wayland-only `$(OBJ): ...header.h` prerequisite rule below expands to
# a long list of real object filenames, which would otherwise silently
# become the first rule (and therefore the default goal) instead of `all`
.DEFAULT_GOAL := all

BACKEND?=xcb

NAME=libmlx.so
SRC_GENERIC=src/mlx_init.c src/mlx_window.c src/mlx_image.c src/mlx_do_sync.c src/mlx_loop.c \
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
	src/backend/mlx__wayland_util.c src/backend/mlx__wayland_wm.c \
	src/backend/mlx__wayland_xdg_shell_protocol.c
# not part of SRC_WAYLAND: only compiled in when detected, see below,
# but always cleaned so a stale .o from an earlier detection never lingers
SRC_WAYLAND_POINTER_WARP=src/backend/mlx__wayland_pointer_warp_protocol.c
SRC_WAYLAND_DECORATION=src/backend/mlx__wayland_decoration_protocol.c
SRC_VULKAN=src/gpu/mlx___vulkan_init.c src/gpu/mlx___vulkan_window.c src/gpu/mlx___vulkan_draw.c \
	src/gpu/mlx___vulkan_image.c

# every object file either backend could ever produce, regardless of the
# BACKEND this particular invocation was made with - `clean` must remove
# all of them, or a stale .o from a previous backend silently looks
# up-to-date to Make (it only compares timestamps, not compiler flags)
# and never gets recompiled for the new backend
ALL_OBJ=$(sort $(patsubst %.c,%.o,$(SRC_GENERIC) $(SRC_XCB) $(SRC_WAYLAND) \
	$(SRC_WAYLAND_POINTER_WARP) $(SRC_WAYLAND_DECORATION) $(SRC_VULKAN)))

SRC=$(SRC_GENERIC)
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
# xdg-decoration is what lets a compositor draw a title bar/borders for
# a plain xdg-shell window; without it a window stays undecorated if
# the compositor has no other client-side-decoration convention either
DECORATION_XML:=$(WAYLAND_PROTOCOLS_DIR)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
ifneq ($(wildcard $(DECORATION_XML)),)
SRC+=$(SRC_WAYLAND_DECORATION)
CFLAGS+=-DMLX_WAYLAND_HAVE_DECORATION
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
ifneq ($(wildcard $(DECORATION_XML)),)
$(OBJ): src/backend/mlx__wayland_decoration_protocol.h
endif
endif

#VK_DEBUG=-DVK_DEBUG_LAYER
VK_DEBUG=

INCLUDES=-I./src
CFLAGS+= $(INCLUDES) $(VK_DEBUG) -fPIC -Wall -O3
LDFLAGS+=

CC=clang

LIBS= $(LIBS_BACKEND) -lvulkan -lz


.PHONY: all config clean re pypkg

all: config $(NAME) pypkg

# .PHONY (not a real prerequisite check on configure.sh's mtime): this
# must always actually run configure.sh, since it's the one place that
# reports missing dependencies - a stale/skipped run here would let a
# broken environment silently fall through to a wayland-scanner/clang
# failure instead of configure's clear diagnostic
config:
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

src/backend/mlx__wayland_decoration_protocol.h: $(DECORATION_XML)
	wayland-scanner client-header $< $@

src/backend/mlx__wayland_decoration_protocol.c: $(DECORATION_XML) src/backend/mlx__wayland_decoration_protocol.h
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
	rm -rf $(NAME) $(ALL_OBJ) *~ src/*~ src/backend/*~ src/gpu/*~ venv python/src/mlx/docs/* python/src/mlx/$(NAME) python/dist test/*~ mlx*.whl python/*~ python/src/mlx.egg-info src/backend/mlx__wayland_xdg_shell_protocol.h src/backend/mlx__wayland_xdg_shell_protocol.c src/backend/mlx__wayland_pointer_warp_protocol.h src/backend/mlx__wayland_pointer_warp_protocol.c src/backend/mlx__wayland_decoration_protocol.h src/backend/mlx__wayland_decoration_protocol.c

re: clean all
