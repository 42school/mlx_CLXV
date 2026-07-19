
# MinilibX

The MinilibX or MLX is a library that provides a very simple API to the graphic capabilities. It's designed to be easy enough for early stage coders that could not immediately use standard graphical libraries and windowing systems.

The set of features is limited, on purpose, and is not meant to be extended. The learners have to build more advanced features, hence developping their own coding skills.


## Installation
MinilibX supports three backends: XCB (default) and Wayland on Linux, and AppKit on macOS. XCB requires vulkan, xcb, xcb-keysyms, bsd, zlib. Wayland requires vulkan, wayland-client, wayland-cursor, xkbcommon, and the wayland-protocols package (+ wayland-scanner) at build time. AppKit requires Xcode's Command Line Tools and a Vulkan implementation with MoltenVK (e.g. via Homebrew, or the LunarG Vulkan SDK).

Not sure which backend your system can build, or want it picked for you? Run `./configure.sh` first, on its own, before `make`:
- on macOS, AppKit is selected automatically if its dependencies are available
- on Linux, if only one of XCB/Wayland's dependencies is available, it's selected automatically; if both are available, you're asked which one to build
- if nothing is available, you're told exactly what to install

The choice is remembered for the next plain `make`. This has to be a separate step before `make`, the same way a classic `./configure && make` works: `make` needs to already know which backend's sources/flags to use before it can run anything, including configure.sh itself.

If you already know which backend you want, skip `./configure.sh` and just run `make` (defaults to XCB), `make BACKEND=wayland`, or `make BACKEND=appkit`. Either way, `make` checks the configuration, compiles the library, and packages the Python module. Edit the Makefile in order to enable the Vulkan validation layers.

## Usage
Read the associated manuals in the `man/` directory, starting with `man man/man3/mlx.3`
Also, the include file mlx.h contains the detailed interface for the MLX library.

## Python Wrapper
The Python directory contains a Mlx class that can be imported in your Python program, and provides access to all library functions described in mlx.h . Some functions require int references to pass back some values, in Python this is converted into the class function returning a tuple. Use the `mlx-*-py3-none-any.whl` package produced after `make` (in the repo root), with the following command `pip install mlx-*-py3-none-any.whl` in your virtualenv.


Created by Olivier Crouzet - 2000-2025
