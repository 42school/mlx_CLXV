# MinilibX Makefile
# Just run `make` - defaults to the XCB backend.
# To build the Wayland backend instead: `make BACKEND=wayland`
# To build the macOS/AppKit backend instead: `make BACKEND=appkit`
#
# Not sure which backend your system can build, or want one picked for
# you automatically? Run `./configure.sh` (standalone, not through make)
# first: it detects what's available and either builds automatically (one
# option available), asks you (both available), or tells you what to
# install (neither available) - then remembers the choice in
# .mlx_config.mk for plain `make` to pick up. This can't happen within a
# single `make` invocation: Make resolves the whole dependency graph
# (which backend's sources/flags to use) before running any recipe,
# including configure.sh - same reason a plain `./configure && make`
# requires configure to run to completion before the build starts.
#
# If a Wayland build dependency is missing (e.g. wayland-protocols), run
# `make BACKEND=wayland config` to see configure.sh's diagnostic without
# attempting the actual build (which would otherwise fail with Make's own
# opaque "No rule to make target" error instead of a clear message).

# set explicitly rather than relying on "first rule in the file" - the
# Wayland-only `$(OBJ): ...header.h` prerequisite rule below expands to
# a long list of real object filenames, which would otherwise silently
# become the first rule (and therefore the default goal) instead of `all`
.DEFAULT_GOAL := all

# picked up from a prior standalone ./configure.sh run, if any (see above)
-include .mlx_config.mk

BACKEND?=xcb

NAME=libmlx.so
SRC_GENERIC=src/mlx_init.c src/mlx_window.c src/mlx_image.c src/mlx_do_sync.c src/mlx_loop.c \
	src/mlx_key_hook.c src/mlx_mouse_hook.c src/mlx_expose_hook.c src/mlx_loop_hook.c \
	src/mlx_hook.c src/mlx_be_gpu_hooks.c src/mlx_xpm.c src/mlx_png.c src/mlx_string_put.c \
	src/mlx_font.c src/mlx_be_extra.c
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
SRC_WAYLAND_CONSTRAINTS=src/backend/mlx__wayland_constraints_protocol.c
SRC_WAYLAND_DECORATION=src/backend/mlx__wayland_decoration_protocol.c
SRC_APPKIT=src/backend/mlx__appkit_init.m src/backend/mlx__appkit_window.m \
	src/backend/mlx__appkit_util.m src/backend/mlx__appkit_event.m \
	src/backend/mlx__appkit_hook.m src/backend/mlx__appkit_flush.m \
	src/backend/mlx__appkit_extra.m
SRC_VULKAN=src/gpu/mlx___vulkan_init.c src/gpu/mlx___vulkan_window.c src/gpu/mlx___vulkan_draw.c \
	src/gpu/mlx___vulkan_image.c

UNAME_S:=$(shell uname -s)

SRC=$(SRC_GENERIC)
ifeq ($(BACKEND),appkit)
SRC+=$(SRC_APPKIT) $(SRC_VULKAN)
CFLAGS+=-DMLX_BACKEND=MLX_BACKEND_APPKIT
LIBS_BACKEND=-framework Cocoa -framework QuartzCore -framework ApplicationServices
ifeq ($(UNAME_S),Darwin)
# macOS has no single standard Vulkan install location (unlike Linux
# distro packages): the LunarG SDK sets $VULKAN_SDK (via its
# setup-env.sh); otherwise, if Homebrew is installed, ask it directly
# for its prefix with `brew --prefix` rather than guessing a path (its
# default differs by CPU architecture, and can be customized besides)
BREW_PREFIX:=$(shell command -v brew >/dev/null 2>&1 && brew --prefix 2>/dev/null)
VULKAN_HEADERS_CANDIDATES:=$(VULKAN_SDK) $(if $(BREW_PREFIX),$(BREW_PREFIX)/opt/vulkan-headers $(BREW_PREFIX))
VULKAN_LOADER_CANDIDATES:=$(VULKAN_SDK) $(if $(BREW_PREFIX),$(BREW_PREFIX)/opt/vulkan-loader $(BREW_PREFIX))
VULKAN_HEADERS_PREFIX:=$(firstword $(foreach p,$(VULKAN_HEADERS_CANDIDATES),$(if $(wildcard $(p)/include/vulkan/vulkan.h),$(p))))
VULKAN_LOADER_PREFIX:=$(firstword $(foreach p,$(VULKAN_LOADER_CANDIDATES),$(if $(wildcard $(p)/lib/libvulkan.dylib),$(p))))
ifneq ($(VULKAN_HEADERS_PREFIX),)
CFLAGS+=-I$(VULKAN_HEADERS_PREFIX)/include
endif
ifneq ($(VULKAN_LOADER_PREFIX),)
LDFLAGS+=-L$(VULKAN_LOADER_PREFIX)/lib -Wl,-rpath,$(VULKAN_LOADER_PREFIX)/lib
endif
endif
else ifeq ($(BACKEND),wayland)
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
# pointer-constraints (lock + set_cursor_position_hint + unlock) is a
# much older/more broadly supported fallback for mlx_mouse_move() than
# pointer-warp-v1 - it's what Xwayland itself uses to emulate XWarpPointer
CONSTRAINTS_XML:=$(WAYLAND_PROTOCOLS_DIR)/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml
ifneq ($(wildcard $(CONSTRAINTS_XML)),)
SRC+=$(SRC_WAYLAND_CONSTRAINTS)
CFLAGS+=-DMLX_WAYLAND_HAVE_POINTER_CONSTRAINTS
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

OBJ=$(patsubst %.m,%.o,$(SRC:.c=.o))

# explicit rule for the AppKit backend's Objective-C sources (MRC, not
# ARC: plain C structs in mlx__appkit_internal.h hold raw object
# pointers) - overrides Make's built-in .m.o rule, which uses $(OBJC)/
# $(OBJCFLAGS) instead of our own $(CC)/$(CFLAGS)
%.o: %.m
	$(CC) $(CFLAGS) -fno-objc-arc -c -o $@ $<

ifeq ($(BACKEND),wayland)
# every wayland source transitively includes the generated xdg-shell
# client header, make sure it exists before any of them gets compiled
$(OBJ): src/backend/mlx__wayland_xdg_shell_protocol.h
ifneq ($(wildcard $(POINTER_WARP_XML)),)
$(OBJ): src/backend/mlx__wayland_pointer_warp_protocol.h
endif
ifneq ($(wildcard $(CONSTRAINTS_XML)),)
$(OBJ): src/backend/mlx__wayland_constraints_protocol.h
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

# pypkg (the optional Python wheel) needs bash (pybuild.sh) and python3;
# configure.sh already reports this, but `all` must not list pypkg as a
# hard prerequisite when they're missing, or the C library build itself
# would fail alongside the (expected, already-reported) skip
HAVE_BASH:=$(shell command -v bash 2>/dev/null)
HAVE_PYTHON3:=$(shell command -v python3 2>/dev/null)
ifneq ($(HAVE_BASH),)
ifneq ($(HAVE_PYTHON3),)
BUILD_PYMOD=1
endif
endif

ifeq ($(BUILD_PYMOD),1)
all: config $(NAME) pypkg
else
all: config $(NAME)
	@echo "Skipping the Python module: bash and/or python3 not found (see configure.sh above)"
endif

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

src/backend/mlx__wayland_constraints_protocol.h: $(CONSTRAINTS_XML)
	wayland-scanner client-header $< $@

src/backend/mlx__wayland_constraints_protocol.c: $(CONSTRAINTS_XML) src/backend/mlx__wayland_constraints_protocol.h
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
	cp version python/version
	./pybuild.sh
	cp python/dist/mlx*.whl .

# wildcarded on directory rather than enumerated from the SRC_* lists
# above, on purpose: a future backend/GPU (say src/backend/mlx__nswindow_*.c
# + src/gpu/mlx___metal_*.c) is cleaned up for free, with nothing to add
# here - same for src/backend/*_protocol.{h,c}, the generated Wayland
# protocol bindings, whatever protocols get added later
clean:
	rm -rf $(NAME) src/*.o src/backend/*.o src/gpu/*.o \
		src/backend/*_protocol.h src/backend/*_protocol.c \
		*~ src/*~ src/backend/*~ src/gpu/*~ venv python/src/mlx/docs/* \
		python/src/mlx/$(NAME) python/dist test/*~ mlx*.whl python/*~ \
		python/src/mlx.egg-info python/version

re: clean all
