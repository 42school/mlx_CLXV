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

# --- Vérifications ---

check_header "vulkan/vulkan.h" "vulkan-headers"
check_lib "vulkan" "vulkan-loader-devel"

check_header "xcb/xcb.h" "libxcbdevel"
check_lib "xcb" "libxcb-devel"

check_header "xcb/xcb_keysyms.h" "xcb-util-keysyms-devel"
check_lib "xcb-keysyms" "xcb-util-keysyms-devel"

check_header "vulkan/vulkan_xcb.h" "vulkan-headers"

check_header "zlib.h" "zlib-ng-compat-devel"
check_lib "z" "zlib-ng-compat-devel"

check_header "bsd/bsd.h" "libbsd-devel"
check_lib "bsd" "libbsd-devel"

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
