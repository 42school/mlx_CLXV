#!/bin/sh

echo "Checking system configuration for your library..."
echo

# Liste des dépendances à tester
DEPS_OK=1
MISSING=""

# Fonction de test d'un header
check_header() {
    HEADER="$1"
	PKG="$2"
    /bin/echo -n "Checking for header <$HEADER>... "
    echo "#include <$HEADER>" | ${CC:-cc} -E - >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "found"
    else
        echo "not found"
        DEPS_OK=0
        MISSING="$MISSING\n  - include: <$HEADER>\t\t=> $PKG"
    fi
}

# Fonction de test d'une librairie (linkage)
check_lib() {
    LIBNAME="$1"
    PKG="$2"  # package probable
    /bin/echo -n "Checking for library -l$LIBNAME... "
    echo "int main() { return 0; }" | ${CC:-cc} -x c - -l$LIBNAME >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "found"
    else
        echo "not found"
        DEPS_OK=0
        MISSING="$MISSING\n  - library: -l$LIBNAME\t\t=> $PKG"
    fi
}

# Function to test for a build tool
check_tool() {
    TOOL="$1"
    PKG="$2"
    /bin/echo -n "Checking for $TOOL... "
    if command -v "$TOOL" >/dev/null 2>&1; then
        echo "found"
    else
        echo "not found"
        DEPS_OK=0
        MISSING="$MISSING\n  - tool: $TOOL\t\t=> $PKG"
    fi
}

# Dependencies needed by every backend (Vulkan itself, zlib for PNG)
check_common() {
    DEPS_OK=1
    MISSING=""
    check_header "vulkan/vulkan.h" "vulkan-headers"
    check_lib "vulkan" "vulkan-loader-devel"
    check_header "zlib.h" "zlib-ng-compat-devel"
    check_lib "z" "zlib-ng-compat-devel"
    COMMON_OK=$DEPS_OK
    COMMON_MISSING="$MISSING"
}

check_xcb() {
    DEPS_OK=1
    MISSING=""
    check_header "xcb/xcb.h" "libxcb-devel"
    check_lib "xcb" "libxcb-devel"
    check_header "xcb/xcb_keysyms.h" "xcb-util-keysyms-devel"
    check_lib "xcb-keysyms" "xcb-util-keysyms-devel"
    check_header "vulkan/vulkan_xcb.h" "vulkan-headers"
    check_header "bsd/bsd.h" "libbsd-devel"
    check_lib "bsd" "libbsd-devel"
    XCB_OK=$DEPS_OK
    XCB_MISSING="$MISSING"
}

check_wayland() {
    DEPS_OK=1
    MISSING=""
    check_header "wayland-client.h" "wayland-devel"
    check_lib "wayland-client" "wayland-devel"
    check_header "wayland-cursor.h" "wayland-devel"
    check_lib "wayland-cursor" "wayland-devel"
    check_header "xkbcommon/xkbcommon.h" "libxkbcommon-devel"
    check_lib "xkbcommon" "libxkbcommon-devel"
    check_header "vulkan/vulkan_wayland.h" "vulkan-headers"
    check_tool "wayland-scanner" "wayland-scanner (often in wayland-devel or wayland-utils)"
    check_tool "pkg-config" "pkgconf / pkg-config"
    if command -v pkg-config >/dev/null 2>&1 && ! pkg-config --exists wayland-protocols; then
        echo "Checking for pkg-config module wayland-protocols... not found"
        DEPS_OK=0
        MISSING="$MISSING\n  - pkg-config module: wayland-protocols\t\t=> wayland-protocols-devel"
    fi
    WAYLAND_OK=$DEPS_OK
    WAYLAND_MISSING="$MISSING"

    # optional extensions: never block the build, just enable/disable a feature
    if command -v pkg-config >/dev/null 2>&1; then
        WAYLAND_PROTOCOLS_DIR=$(pkg-config --variable=pkgdatadir wayland-protocols 2>/dev/null)

        /bin/echo -n "Checking for pointer-warp-v1 protocol (optional, for mlx_mouse_move)... "
        if [ -n "$WAYLAND_PROTOCOLS_DIR" ] && [ -f "$WAYLAND_PROTOCOLS_DIR/staging/pointer-warp/pointer-warp-v1.xml" ]; then
            echo "found"
        else
            echo "not found"
        fi

        /bin/echo -n "Checking for pointer-constraints protocol (optional fallback for mlx_mouse_move)... "
        if [ -n "$WAYLAND_PROTOCOLS_DIR" ] && [ -f "$WAYLAND_PROTOCOLS_DIR/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml" ]; then
            echo "found"
        else
            echo "not found - mlx_mouse_move() will be unsupported on this backend"
        fi

        /bin/echo -n "Checking for xdg-decoration protocol (optional, for window decoration)... "
        if [ -n "$WAYLAND_PROTOCOLS_DIR" ] && [ -f "$WAYLAND_PROTOCOLS_DIR/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml" ]; then
            echo "found"
        else
            echo "not found - windows will be undecorated on this backend"
        fi
    fi
}

report_ok() {
    echo "✅ All required headers and libraries are available for $1."
}

report_missing() {
    echo "❌ Missing dependencies for $1:"
    printf '%b\n' "$2"
}

# --- platform ---

PLATFORM=$(uname -s)
echo "Platform: $PLATFORM"
if [ "$PLATFORM" != "Linux" ]; then
    echo "⚠️  Both backends assume Linux (X11/Wayland protocols, memfd_create,"
    echo "    evdev keycodes...); this may not build or run correctly on $PLATFORM."
fi
echo

check_common
if [ "$COMMON_OK" -eq 0 ]; then
    report_missing "every backend" "$COMMON_MISSING"
    echo
    echo "Install these first (needed no matter which backend you pick), then"
    echo "re-run ./configure.sh."
    rm -f a.out
    exit 1
fi
echo

# `make` always passes a concrete BACKEND (defaulting to xcb, see Makefile),
# so this only auto-detects when configure.sh is run directly, standalone,
# without a pre-set BACKEND environment variable
if [ -n "$BACKEND" ]; then
    echo "Backend: $BACKEND (explicitly requested)"
    echo
    if [ "$BACKEND" = "wayland" ]; then
        check_wayland
        OK=$WAYLAND_OK
        MISS="$WAYLAND_MISSING"
    else
        check_xcb
        OK=$XCB_OK
        MISS="$XCB_MISSING"
    fi
    rm -f a.out
    echo
    if [ "$OK" -eq 1 ]; then
        report_ok "$BACKEND"
        exit 0
    fi
    report_missing "the $BACKEND backend" "$MISS"
    echo
    echo "You may need to install missing packages or specify include/library paths:"
    echo "  e.g.  ./configure.sh CFLAGS='-I/path/to/include' LDFLAGS='-L/path/to/lib'"
    exit 1
fi

echo "No backend requested (BACKEND is not set) - checking both and deciding..."
echo
check_xcb
echo
check_wayland
echo
rm -f a.out

if [ "$XCB_OK" -eq 1 ] && [ "$WAYLAND_OK" -eq 1 ]; then
    echo "Both XCB and Wayland dependencies are available."
    if [ -t 0 ]; then
        /bin/echo -n "Which backend would you like to build? [xcb/wayland] (default: xcb): "
        read CHOICE
        case "$CHOICE" in
            wayland|Wayland|WAYLAND) CHOSEN=wayland ;;
            *) CHOSEN=xcb ;;
        esac
    else
        echo "Not running in a terminal, so I can't ask - defaulting to xcb."
        echo "Run ./configure.sh from a terminal to be asked, or force a choice with"
        echo "  BACKEND=wayland ./configure.sh"
        CHOSEN=xcb
    fi
elif [ "$XCB_OK" -eq 1 ]; then
    echo "Only XCB's dependencies are available - selecting it."
    CHOSEN=xcb
elif [ "$WAYLAND_OK" -eq 1 ]; then
    echo "Only Wayland's dependencies are available - selecting it."
    CHOSEN=wayland
else
    echo "❌ Neither backend's dependencies are fully available."
    echo
    report_missing "XCB" "$XCB_MISSING"
    echo
    report_missing "Wayland" "$WAYLAND_MISSING"
    echo
    echo "Install what you need for at least one of them, then re-run ./configure.sh."
    exit 1
fi

echo "BACKEND=$CHOSEN" > .mlx_config.mk
echo
echo "✅ Selected backend: $CHOSEN (remembered in .mlx_config.mk)"
echo "Run 'make' to build it (or 'make BACKEND=xcb'/'make BACKEND=wayland' to override)."
exit 0
