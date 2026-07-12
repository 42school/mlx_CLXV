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

# --- Vérifications ---

BACKEND=${BACKEND:-xcb}
echo "Backend: $BACKEND"
echo

check_header "vulkan/vulkan.h" "vulkan-headers"
check_lib "vulkan" "vulkan-loader-devel"

check_header "zlib.h" "zlib-ng-compat-devel"
check_lib "z" "zlib-ng-compat-devel"

if [ "$BACKEND" = "wayland" ]; then
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

    # optional: only enables mlx_mouse_move(), not required to build
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

        # optional: only enables a compositor-drawn title bar, not required to build
        /bin/echo -n "Checking for xdg-decoration protocol (optional, for window decoration)... "
        if [ -n "$WAYLAND_PROTOCOLS_DIR" ] && [ -f "$WAYLAND_PROTOCOLS_DIR/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml" ]; then
            echo "found"
        else
            echo "not found - windows will be undecorated on this backend"
        fi
    fi
else
    check_header "xcb/xcb.h" "libxcbdevel"
    check_lib "xcb" "libxcb-devel"

    check_header "xcb/xcb_keysyms.h" "xcb-util-keysyms-devel"
    check_lib "xcb-keysyms" "xcb-util-keysyms-devel"

    check_header "vulkan/vulkan_xcb.h" "vulkan-headers"

    check_header "bsd/bsd.h" "libbsd-devel"
    check_lib "bsd" "libbsd-devel"
fi

rm -f a.out

# --- Résumé final ---
echo
if [ $DEPS_OK -eq 1 ]; then
    echo "✅ All required headers and libraries are available."
    exit 0
else
    echo "❌ Some dependencies are missing or not accessible:"
    echo -e "$MISSING"
    echo
    echo "You may need to install missing packages or specify include/library paths:"
    echo "  e.g.  ./configure.sh CFLAGS='-I/path/to/include' LDFLAGS='-L/path/to/lib'"
    exit 1
fi
