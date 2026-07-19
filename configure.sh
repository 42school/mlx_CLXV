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
    echo "#include <$HEADER>" | ${CC:-cc} $CFLAGS -E - >/dev/null 2>&1
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
    echo "int main() { return 0; }" | ${CC:-cc} -x c - $CFLAGS $LDFLAGS -l$LIBNAME >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "found"
    else
        echo "not found"
        DEPS_OK=0
        MISSING="$MISSING\n  - library: -l$LIBNAME\t\t=> $PKG"
    fi
}

# macOS has no single standard Vulkan install location (unlike Linux
# distro packages): the LunarG SDK sets $VULKAN_SDK (via its
# setup-env.sh); otherwise, if Homebrew is installed, ask it directly
# for its prefix with `brew --prefix` rather than guessing a path (its
# default differs by CPU architecture, and can be customized besides) -
# BREW_PREFIX is set once, below, before this is ever called
find_darwin_prefix() {
    FORMULA="$1"
    CHECK_PATH="$2"
    CANDIDATES="$VULKAN_SDK"
    if [ -n "$BREW_PREFIX" ]; then
        CANDIDATES="$CANDIDATES $BREW_PREFIX/opt/$FORMULA $BREW_PREFIX"
    fi
    for p in $CANDIDATES; do
        if [ -n "$p" ] && [ -e "$p/$CHECK_PATH" ]; then
            echo "$p"
            return 0
        fi
    done
    return 1
}

# Fonction de test d'un framework Apple (pas de header/lib au sens Linux)
check_framework() {
    FRAMEWORK="$1"
    PKG="$2"
    /bin/echo -n "Checking for framework $FRAMEWORK... "
    echo "int main(){return 0;}" | ${CC:-cc} -x objective-c - -framework "$FRAMEWORK" -o /tmp/mlx_ak_check.$$ >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "found"
        rm -f /tmp/mlx_ak_check.$$
    else
        echo "not found"
        DEPS_OK=0
        MISSING="$MISSING\n  - framework: $FRAMEWORK\t\t=> $PKG"
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

# Dependencies needed by every backend (a C compiler, make, Vulkan, zlib)
check_common() {
    DEPS_OK=1
    MISSING=""
    check_tool "${CC:-cc}" "a C compiler (clang or gcc)"
    check_tool "make" "make"
    check_header "vulkan/vulkan.h" "vulkan-headers"
    check_lib "vulkan" "vulkan-loader-devel"
    check_header "zlib.h" "zlib-ng-compat-devel"
    check_lib "z" "zlib-ng-compat-devel"
    COMMON_OK=$DEPS_OK
    COMMON_MISSING="$MISSING"
}

# Only needed for `make`'s pypkg step (packaging the Python wheel); never
# blocks the C library itself, so this is checked and reported separately
check_python_module() {
    DEPS_OK=1
    MISSING=""
    check_tool "bash" "bash"
    check_tool "python3" "python3"
    PYMOD_OK=$DEPS_OK
    PYMOD_MISSING="$MISSING"
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

check_appkit() {
    DEPS_OK=1
    MISSING=""
    check_framework "Cocoa" "Xcode Command Line Tools (xcode-select --install)"
    check_framework "QuartzCore" "Xcode Command Line Tools (xcode-select --install)"
    check_framework "ApplicationServices" "Xcode Command Line Tools (xcode-select --install)"
    check_header "vulkan/vulkan_metal.h" "vulkan-headers"
    check_lib "vulkan" "vulkan-loader"
    /bin/echo -n "Checking for MoltenVK (Vulkan-on-Metal driver)... "
    MOLTENVK_PREFIX=$(find_darwin_prefix molten-vk lib/libMoltenVK.dylib)
    if [ -n "$MOLTENVK_PREFIX" ]; then
        echo "found ($MOLTENVK_PREFIX)"
    else
        echo "not found"
        DEPS_OK=0
        MISSING="$MISSING\n  - library: libMoltenVK.dylib\t\t=> molten-vk (via Homebrew: brew install molten-vk; or the LunarG Vulkan SDK)"
    fi
    APPKIT_OK=$DEPS_OK
    APPKIT_MISSING="$MISSING"
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
if [ "$PLATFORM" = "Darwin" ]; then
    echo "  -> use the AppKit backend on macOS: make BACKEND=appkit"
    BREW_PREFIX=""
    command -v brew >/dev/null 2>&1 && BREW_PREFIX=$(brew --prefix 2>/dev/null)
    # find_darwin_prefix (defined above) so check_header/check_lib below
    # (and check_appkit's own checks) actually find things, regardless of
    # whether Vulkan came from the LunarG SDK, Homebrew, or a manual copy
    VULKAN_HEADERS_PREFIX=$(find_darwin_prefix vulkan-headers include/vulkan/vulkan.h)
    VULKAN_LOADER_PREFIX=$(find_darwin_prefix vulkan-loader lib/libvulkan.dylib)
    [ -n "$VULKAN_HEADERS_PREFIX" ] && CFLAGS="$CFLAGS -I$VULKAN_HEADERS_PREFIX/include"
    [ -n "$VULKAN_LOADER_PREFIX" ] && LDFLAGS="$LDFLAGS -L$VULKAN_LOADER_PREFIX/lib"
elif [ "$PLATFORM" != "Linux" ]; then
    echo "⚠️  The XCB and Wayland backends assume Linux (X11/Wayland protocols,"
    echo "    memfd_create, evdev keycodes...); this may not build or run"
    echo "    correctly on $PLATFORM. The AppKit backend targets macOS only."
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

check_python_module
if [ "$PYMOD_OK" -eq 0 ]; then
    echo "⚠️  The MLX Python module won't be built, some tools are missing:"
    printf '%b\n' "$PYMOD_MISSING"
    echo "   (the C library itself is unaffected, only the optional Python wheel)"
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
    elif [ "$BACKEND" = "appkit" ]; then
        check_appkit
        OK=$APPKIT_OK
        MISS="$APPKIT_MISSING"
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

echo "No backend requested (BACKEND is not set) - checking what's available..."
echo

if [ "$PLATFORM" = "Darwin" ]; then
    check_appkit
    echo
    rm -f a.out
    if [ "$APPKIT_OK" -eq 1 ]; then
        echo "AppKit's dependencies are available - selecting it."
        CHOSEN=appkit
    else
        echo "❌ The AppKit backend's dependencies are not fully available."
        echo
        report_missing "AppKit" "$APPKIT_MISSING"
        echo
        echo "Install what you need, then re-run ./configure.sh."
        exit 1
    fi
    echo "BACKEND=$CHOSEN" > .mlx_config.mk
    echo
    echo "✅ Selected backend: $CHOSEN (remembered in .mlx_config.mk)"
    echo "Run 'make' to build it."
    exit 0
fi

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
