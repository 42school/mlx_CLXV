
# Mlx

A Python package to access the MinilibX functionalities

## Features

- Open a window on screen
- Draw in it directly, pixel per pixel
- Manipulate images for faster rendering
- Get keyboard and mouse user inputs
- Load png and xpm files

## Installation
In your preferred virtual environment:
```bash
pip install mlx_CLXV-2.2-py3-none-any.whl
```

## Quick Start
```python
from Mlx import Mlx

def mymouse(button, x, y, mystuff):
    print(f"Got mouse event! button {button} at {x},{y}.")

def mykey(keynum, mystuff):
    print(f"Got key {keynum}, and got my stuff back:")
    print(mystuff)
    if keynum == 32:
        m.mlx_mouse_hook(win_ptr, None, None)

m = Mlx()
mlx_ptr = m.mlx_init()
win_ptr = m.mlx_new_window(mlx_ptr, 200, 200, "toto")
m.mlx_clear_window(mlx_ptr, win_ptr)
m.mlx_string_put(mlx_ptr, win_ptr, 20, 20, 255, "Hello PyMlx!")
(ret, w, h) = m.mlx_get_screen_size(mlx_ptr)
print(f"Got screen size: {w} x {h} .")

stuff = [1, 2]
m.mlx_mouse_hook(win_ptr, mymouse, None)
m.mlx_key_hook(win_ptr, mykey, stuff)

m.mlx_loop(mlx_ptr)
```

## API

The Python Mlx is only a wrapper to the C Mlx library. You will
find in the *docs* subdirectory the man mages and the include
file *mlx.h* .
All methods from the Mlx Python class match the names of the
C functions from the library. The parameters are identical,
except for pointers used to send back information to the caller.
In that case, the Python Mlx method returns a tuple.
Example:
- int mlx_get_screen_size(void *mlx_ptr, unsigned int *width, unsigned int *height);
This function expects 2 uint pointers to fill them, back to the caller.
- mlx_get_screen_size(mlx_ptr)
The Python equivalent only takes 1 parameter instead of 3, but returns a tuple with the
return value and the 2 unsigned ints.
