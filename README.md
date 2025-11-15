
# MinilibX

The MinilibX or MLX is a library that provides a very simple API to the graphic capabilities. It's designed to be easy enough for early stage coders that could not immediately use standard graphical libraries and windowing systems.

The set of features is limited, on purpose, and is not meant to be extended. The learners have to build more advanced features, hence developping their own coding skills.


## Installation
The current MinilibX version is designed to work under Linux, and requires the runtime and developper packages: vulkan, xcb, xcb-keysyms, bsd, zip.

Just use `make` to check the configuration, compile the library, and package the Python module. Edit the Makefile in order to enable the Vulkan validation layers.

## Usage
Read the associated manuals in the `man/` directory, starting with `man man/man3/mlx.3`
Also, the include file mlx.h contains the detailed interface for the MLX library.

## Python Wrapper
The Python directory contains a Mlx class that can be imported in your Python program, and provides access to all library functions described in mlx.h . Some functions require int references to pass back some values, in Python this is converted into the class function returning a tuple. Use `mlx-2.2-py3-none-any.whl` package after `make`, with the following command `pip install mlx-2.2-py3-none-any.whl` in your virtualenv.


Created by Olivier Crouzet - 2000-2025
