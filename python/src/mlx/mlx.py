"""Python wrapper for the MLX C library.

See the mlx manuals and ``mlx.h`` from the C library for the detailed
semantics of each function. Functions that need to hand back extra
values through pointer arguments in C (e.g. width/height, x/y) are
converted here into Python methods that return a tuple instead.
"""

import ctypes
import os
import sys
from typing import Callable, Dict, Optional, Tuple

#: Callback signature for :meth:`Mlx.mlx_mouse_hook`: button, x, y, param.
MouseCallback = Callable[[int, int, int, object], object]
#: Callback signature for :meth:`Mlx.mlx_key_hook`: keycode, param.
KeyCallback = Callable[[int, object], object]
#: Callback signature for :meth:`Mlx.mlx_expose_hook`/``mlx_loop_hook``: param.
NoArgCallback = Callable[[object], object]
#: Callback signature accepted by the generic :meth:`Mlx.mlx_hook`; its
#: actual arity depends on ``x_event`` and can't be expressed statically.
HookCallback = Callable[..., object]


class Mlx:
    """Thin ctypes wrapper exposing every function documented in mlx.h."""

    def __init__(self) -> None:
        """Load the MLX shared library shipped alongside this module."""
        module_dir = os.path.dirname(os.path.abspath(__file__))
        # the AppKit backend's Makefile builds libmlx.dylib (macOS's
        # native shared-library convention), every other backend builds
        # libmlx.so
        libname = "libmlx.dylib" if sys.platform == "darwin" else "libmlx.so"
        self.so_file = os.path.join(module_dir, libname)
        self.mlx_func: ctypes.CDLL = ctypes.CDLL(self.so_file)
        self._python_ref_std: Dict[str, object] = {}
        self._python_ref_gen: Dict[str, object] = {}
        self._img_height: Dict[str, int] = {}

    # Initialisation

    def mlx_init(self) -> Optional[int]:
        """Initialize the library. Must be called before anything else."""
        self.mlx_func.mlx_init.restype = ctypes.c_void_p
        result: Optional[int] = self.mlx_func.mlx_init()
        return result

    def mlx_release(self, mlx_ptr: Optional[int]) -> int:
        """Release every resource held by ``mlx_ptr``."""
        self.mlx_func.mlx_release.argtypes = [ctypes.c_void_p]
        self.mlx_func.mlx_release.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_release(mlx_ptr)
        return result

    # Windows

    def mlx_new_window(
        self, mlx_ptr: Optional[int], width: int, height: int, title: str
    ) -> Optional[int]:
        """Create a new window of the given size and title."""
        self.mlx_func.mlx_new_window.argtypes = [
            ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint, ctypes.c_char_p,
        ]
        self.mlx_func.mlx_new_window.restype = ctypes.c_void_p
        result: Optional[int] = self.mlx_func.mlx_new_window(
            mlx_ptr, width, height, title.encode("utf-8")
        )
        return result

    def mlx_clear_window(
        self, mlx_ptr: Optional[int], win_ptr: Optional[int]
    ) -> int:
        """Clear the whole window to black."""
        self.mlx_func.mlx_clear_window.argtypes = [
            ctypes.c_void_p, ctypes.c_void_p,
        ]
        self.mlx_func.mlx_clear_window.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_clear_window(mlx_ptr, win_ptr)
        return result

    def mlx_pixel_put(
        self,
        mlx_ptr: Optional[int],
        win_ptr: Optional[int],
        x: int,
        y: int,
        color: int,
    ) -> int:
        """Set a single pixel of the window to ``color``."""
        self.mlx_func.mlx_pixel_put.argtypes = [
            ctypes.c_void_p, ctypes.c_void_p,
            ctypes.c_uint, ctypes.c_uint, ctypes.c_uint,
        ]
        self.mlx_func.mlx_pixel_put.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_pixel_put(
            mlx_ptr, win_ptr, x, y, color
        )
        return result

    def mlx_destroy_window(
        self, mlx_ptr: Optional[int], win_ptr: Optional[int]
    ) -> int:
        """Destroy a window created by :meth:`mlx_new_window`."""
        self.mlx_func.mlx_destroy_window.argtypes = [
            ctypes.c_void_p, ctypes.c_void_p,
        ]
        self.mlx_func.mlx_destroy_window.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_destroy_window(mlx_ptr, win_ptr)
        return result

    # Images

    def mlx_new_image(
        self, mlx_ptr: Optional[int], width: int, height: int
    ) -> Optional[int]:
        """Create a new off-screen image of the given size."""
        self.mlx_func.mlx_new_image.argtypes = [
            ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint,
        ]
        self.mlx_func.mlx_new_image.restype = ctypes.c_void_p
        ret: Optional[int] = self.mlx_func.mlx_new_image(
            mlx_ptr, width, height
        )
        if ret is not None:
            self._img_height[str(ret)] = height
        return ret

    def mlx_get_data_addr(
        self, img_ptr: Optional[int]
    ) -> Tuple[memoryview, int, int, int]:
        """Return a writable ``(pixels, bits_per_pixel, size_line, format)``.

        API break: returns a tuple instead of writing through pointers.
        """
        bits_per_pixel = ctypes.c_uint()
        size_line = ctypes.c_uint()
        theformat = ctypes.c_uint()
        self.mlx_func.mlx_get_data_addr.argtypes = [
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_uint),
            ctypes.POINTER(ctypes.c_uint),
            ctypes.POINTER(ctypes.c_uint),
        ]
        self.mlx_func.mlx_get_data_addr.restype = ctypes.POINTER(ctypes.c_char)
        data = self.mlx_func.mlx_get_data_addr(
            img_ptr,
            ctypes.byref(bits_per_pixel),
            ctypes.byref(size_line),
            ctypes.byref(theformat),
        )
        data_array = ctypes.c_char * (
            self._img_height[str(img_ptr)] * size_line.value
        )
        data_view = data_array.from_address(ctypes.addressof(data.contents))
        return (
            memoryview(data_view).cast("B"),
            bits_per_pixel.value,
            size_line.value,
            theformat.value,
        )

    def mlx_put_image_to_window(
        self,
        mlx_ptr: Optional[int],
        win_ptr: Optional[int],
        img_ptr: Optional[int],
        x: int,
        y: int,
    ) -> int:
        """Blit an image onto a window at ``(x, y)``."""
        self.mlx_func.mlx_put_image_to_window.argtypes = [
            ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
            ctypes.c_int, ctypes.c_int,
        ]
        self.mlx_func.mlx_put_image_to_window.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_put_image_to_window(
            mlx_ptr, win_ptr, img_ptr, x, y
        )
        return result

    def mlx_destroy_image(
        self, mlx_ptr: Optional[int], img_ptr: Optional[int]
    ) -> int:
        """Destroy an image created by :meth:`mlx_new_image` (or a loader)."""
        self._img_height.pop(str(img_ptr))
        self.mlx_func.mlx_destroy_image.argtypes = [
            ctypes.c_void_p, ctypes.c_void_p,
        ]
        self.mlx_func.mlx_destroy_image.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_destroy_image(mlx_ptr, img_ptr)
        return result

    # Events & main loop
    # Note: Python can't catch Ctrl-C from the keyboard during mlx_loop()
    # execution. Use Ctrl-\ to kill the program instead.

    def mlx_loop(self, mlx_ptr: Optional[int]) -> int:
        """Run the event loop until :meth:`mlx_loop_exit` is called."""
        self.mlx_func.mlx_loop.argtypes = [ctypes.c_void_p]
        self.mlx_func.mlx_loop.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_loop(mlx_ptr)
        return result

    def mlx_loop_exit(self, mlx_ptr: Optional[int]) -> int:
        """Ask a running :meth:`mlx_loop` to return."""
        self.mlx_func.mlx_loop_exit.argtypes = [ctypes.c_void_p]
        self.mlx_func.mlx_loop_exit.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_loop_exit(mlx_ptr)
        return result

    def mlx_mouse_hook(
        self,
        win_ptr: Optional[int],
        callback: Optional[MouseCallback],
        param: object,
    ) -> int:
        """Register (or clear, with ``callback=None``) a mouse-click hook."""
        self.mlx_func.mlx_mouse_hook.restype = ctypes.c_int
        if not callback:
            self._python_ref_std[str(win_ptr) + "_mouse_f"] = None
            self._python_ref_std[str(win_ptr) + "_mouse_p"] = None
            self.mlx_func.mlx_mouse_hook.argtypes = [
                ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
            ]
            result: int = self.mlx_func.mlx_mouse_hook(win_ptr, None, None)
            return result
        callback_type = ctypes.CFUNCTYPE(
            None, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.py_object
        )
        self.mlx_func.mlx_mouse_hook.argtypes = [
            ctypes.c_void_p, callback_type, ctypes.py_object,
        ]
        callback_ref = callback_type(callback)
        self._python_ref_std[str(win_ptr) + "_mouse_f"] = callback_ref
        self._python_ref_std[str(win_ptr) + "_mouse_p"] = param
        result = self.mlx_func.mlx_mouse_hook(win_ptr, callback_ref, param)
        return result

    def mlx_key_hook(
        self,
        win_ptr: Optional[int],
        callback: Optional[KeyCallback],
        param: object,
    ) -> int:
        """Register (or clear, with ``callback=None``) a key-release hook."""
        self.mlx_func.mlx_key_hook.restype = ctypes.c_int
        if not callback:
            self._python_ref_std[str(win_ptr) + "_key_f"] = None
            self._python_ref_std[str(win_ptr) + "_key_p"] = None
            self.mlx_func.mlx_key_hook.argtypes = [
                ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
            ]
            result: int = self.mlx_func.mlx_key_hook(win_ptr, None, None)
            return result
        callback_type = ctypes.CFUNCTYPE(None, ctypes.c_uint, ctypes.py_object)
        self.mlx_func.mlx_key_hook.argtypes = [
            ctypes.c_void_p, callback_type, ctypes.py_object,
        ]
        callback_ref = callback_type(callback)
        self._python_ref_std[str(win_ptr) + "_key_f"] = callback_ref
        self._python_ref_std[str(win_ptr) + "_key_p"] = param
        result = self.mlx_func.mlx_key_hook(win_ptr, callback_ref, param)
        return result

    def mlx_expose_hook(
        self,
        win_ptr: Optional[int],
        callback: Optional[NoArgCallback],
        param: object,
    ) -> int:
        """Register (or clear, with ``callback=None``) an expose hook."""
        self.mlx_func.mlx_expose_hook.restype = ctypes.c_int
        if not callback:
            self._python_ref_std[str(win_ptr) + "_expose_f"] = None
            self._python_ref_std[str(win_ptr) + "_expose_p"] = None
            self.mlx_func.mlx_expose_hook.argtypes = [
                ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
            ]
            result: int = self.mlx_func.mlx_expose_hook(win_ptr, None, None)
            return result
        callback_type = ctypes.CFUNCTYPE(None, ctypes.py_object)
        self.mlx_func.mlx_expose_hook.argtypes = [
            ctypes.c_void_p, callback_type, ctypes.py_object,
        ]
        callback_ref = callback_type(callback)
        self._python_ref_std[str(win_ptr) + "_expose_f"] = callback_ref
        self._python_ref_std[str(win_ptr) + "_expose_p"] = param
        result = self.mlx_func.mlx_expose_hook(win_ptr, callback_ref, param)
        return result

    def mlx_loop_hook(
        self,
        mlx_ptr: Optional[int],
        callback: Optional[NoArgCallback],
        param: object,
    ) -> int:
        """Register (or clear, with ``callback=None``) a per-loop-tick hook."""
        self.mlx_func.mlx_loop_hook.restype = ctypes.c_int
        if not callback:
            self._python_ref_std["loop_f"] = None
            self._python_ref_std["loop_p"] = None
            self.mlx_func.mlx_loop_hook.argtypes = [
                ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
            ]
            result: int = self.mlx_func.mlx_loop_hook(mlx_ptr, None, None)
            return result
        callback_type = ctypes.CFUNCTYPE(None, ctypes.py_object)
        self.mlx_func.mlx_loop_hook.argtypes = [
            ctypes.c_void_p, callback_type, ctypes.py_object,
        ]
        callback_ref = callback_type(callback)
        self._python_ref_std["loop_f"] = callback_ref
        self._python_ref_std["loop_p"] = param
        result = self.mlx_func.mlx_loop_hook(mlx_ptr, callback_ref, param)
        return result

    def mlx_hook(
        self,
        win_ptr: Optional[int],
        x_event: int,
        x_mask: int,
        callback: Optional[HookCallback],
        param: object,
    ) -> int:
        """Register a hook for a raw (X11-numbered) event on a window.

        Unlike the convenience hooks above, the callback's arity depends
        on ``x_event``: key events get ``(keycode, param)``, button
        events get ``(button, x, y, param)``, motion events get
        ``(x, y, param)``, anything else gets just ``(param,)``.
        """
        x_event_key = [2, 3]
        x_event_mouse = [4, 5]
        x_event_motion = [6]
        self.mlx_func.mlx_hook.restype = ctypes.c_int
        if not callback:
            self._python_ref_gen[str(win_ptr) + "_f_" + str(x_event)] = None
            self._python_ref_gen[str(win_ptr) + "_p_" + str(x_event)] = None
            self.mlx_func.mlx_hook.argtypes = [
                ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint,
                ctypes.c_void_p, ctypes.c_void_p,
            ]
            result: int = self.mlx_func.mlx_hook(win_ptr, 0, 0, None, None)
            return result
        if x_event in x_event_key:
            callback_type = ctypes.CFUNCTYPE(
                None, ctypes.c_uint, ctypes.py_object
            )
        elif x_event in x_event_mouse:
            callback_type = ctypes.CFUNCTYPE(
                None,
                ctypes.c_uint, ctypes.c_uint, ctypes.c_uint,
                ctypes.py_object,
            )
        elif x_event in x_event_motion:
            callback_type = ctypes.CFUNCTYPE(
                None, ctypes.c_uint, ctypes.c_uint, ctypes.py_object
            )
        else:
            callback_type = ctypes.CFUNCTYPE(None, ctypes.py_object)

        self.mlx_func.mlx_hook.argtypes = [
            ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint,
            callback_type, ctypes.py_object,
        ]
        callback_ref = callback_type(callback)
        gen_key_f = str(win_ptr) + "_f_" + str(x_event)
        gen_key_p = str(win_ptr) + "_p_" + str(x_event)
        self._python_ref_gen[gen_key_f] = callback_ref
        self._python_ref_gen[gen_key_p] = param
        result = self.mlx_func.mlx_hook(
            win_ptr, x_event, x_mask, callback_ref, param
        )
        return result

    # Misc.

    def mlx_string_put(
        self,
        mlx_ptr: Optional[int],
        win_ptr: Optional[int],
        x: int,
        y: int,
        color: int,
        string: str,
    ) -> int:
        """Draw ``string`` at ``(x, y)`` using the built-in bitmap font."""
        self.mlx_func.mlx_string_put.argtypes = [
            ctypes.c_void_p, ctypes.c_void_p,
            ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_char_p,
        ]
        self.mlx_func.mlx_string_put.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_string_put(
            mlx_ptr, win_ptr, x, y, color, string.encode("utf-8")
        )
        return result

    def mlx_xpm_file_to_image(
        self, mlx_ptr: Optional[int], filename: str
    ) -> Tuple[Optional[int], int, int]:
        """Load an XPM file, returning ``(image, width, height)``.

        API break: returns a tuple instead of writing through pointers.
        """
        width = ctypes.c_uint()
        height = ctypes.c_uint()
        self.mlx_func.mlx_xpm_file_to_image.argtypes = [
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_void_p,
        ]
        self.mlx_func.mlx_xpm_file_to_image.restype = ctypes.c_void_p
        img: Optional[int] = self.mlx_func.mlx_xpm_file_to_image(
            mlx_ptr,
            filename.encode("utf8"),
            ctypes.byref(width),
            ctypes.byref(height),
        )
        if img is not None:
            self._img_height[str(img)] = height.value
        return (img, width.value, height.value)

    def mlx_png_file_to_image(
        self, mlx_ptr: Optional[int], filename: str
    ) -> Tuple[Optional[int], int, int]:
        """Load a PNG file, returning ``(image, width, height)``.

        API break: returns a tuple instead of writing through pointers.
        """
        width = ctypes.c_uint()
        height = ctypes.c_uint()
        self.mlx_func.mlx_png_file_to_image.argtypes = [
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_void_p,
        ]
        self.mlx_func.mlx_png_file_to_image.restype = ctypes.c_void_p
        img: Optional[int] = self.mlx_func.mlx_png_file_to_image(
            mlx_ptr,
            filename.encode("utf8"),
            ctypes.byref(width),
            ctypes.byref(height),
        )
        if img is not None:
            self._img_height[str(img)] = height.value
        return (img, width.value, height.value)

    # mlx_xpm_to_image() (in-memory XPM data) isn't exposed: not really
    # useful in a Python context, students load images from files instead.

    def mlx_mouse_hide(self, mlx_ptr: Optional[int]) -> int:
        """Hide the mouse cursor."""
        self.mlx_func.mlx_mouse_hide.argtypes = [ctypes.c_void_p]
        self.mlx_func.mlx_mouse_hide.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_mouse_hide(mlx_ptr)
        return result

    def mlx_mouse_show(self, mlx_ptr: Optional[int]) -> int:
        """Show the mouse cursor."""
        self.mlx_func.mlx_mouse_show.argtypes = [ctypes.c_void_p]
        self.mlx_func.mlx_mouse_show.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_mouse_show(mlx_ptr)
        return result

    def mlx_mouse_move(self, mlx_ptr: Optional[int], x: int, y: int) -> int:
        """Move the mouse cursor to ``(x, y)`` within the window."""
        self.mlx_func.mlx_mouse_move.argtypes = [
            ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
        ]
        self.mlx_func.mlx_mouse_move.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_mouse_move(mlx_ptr, x, y)
        return result

    def mlx_mouse_get_pos(
        self, mlx_ptr: Optional[int]
    ) -> Tuple[int, int, int]:
        """Return ``(status, x, y)``, the current mouse position.

        API break: returns a tuple instead of writing through pointers.
        """
        x = ctypes.c_int()
        y = ctypes.c_int()
        self.mlx_func.mlx_mouse_get_pos.argtypes = [
            ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
        ]
        self.mlx_func.mlx_mouse_get_pos.restype = ctypes.c_int
        val: int = self.mlx_func.mlx_mouse_get_pos(
            mlx_ptr, ctypes.byref(x), ctypes.byref(y)
        )
        return (val, x.value, y.value)

    def mlx_do_key_autorepeatoff(self, mlx_ptr: Optional[int]) -> int:
        """Disable OS-level keyboard auto-repeat."""
        self.mlx_func.mlx_do_key_autorepeatoff.argtypes = [ctypes.c_void_p]
        self.mlx_func.mlx_do_key_autorepeatoff.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_do_key_autorepeatoff(mlx_ptr)
        return result

    def mlx_do_key_autorepeaton(self, mlx_ptr: Optional[int]) -> int:
        """Re-enable OS-level keyboard auto-repeat."""
        self.mlx_func.mlx_do_key_autorepeaton.argtypes = [ctypes.c_void_p]
        self.mlx_func.mlx_do_key_autorepeaton.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_do_key_autorepeaton(mlx_ptr)
        return result

    def mlx_get_screen_size(
        self, mlx_ptr: Optional[int]
    ) -> Tuple[int, int, int]:
        """Return ``(status, width, height)`` of the screen.

        API break: returns a tuple instead of writing through pointers.
        """
        w = ctypes.c_uint()
        h = ctypes.c_uint()
        uint_p = ctypes.POINTER(ctypes.c_uint)
        self.mlx_func.mlx_get_screen_size.argtypes = [
            ctypes.c_void_p, uint_p, uint_p,
        ]
        self.mlx_func.mlx_get_screen_size.restype = ctypes.c_int
        val: int = self.mlx_func.mlx_get_screen_size(
            mlx_ptr, ctypes.byref(w), ctypes.byref(h)
        )
        return (val, w.value, h.value)

    # Sync functions

    def mlx_do_sync(self, mlx_ptr: Optional[int]) -> int:
        """Flush all pending requests for every window, wait for completion."""
        self.mlx_func.mlx_do_sync.argtypes = [ctypes.c_void_p]
        self.mlx_func.mlx_do_sync.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_do_sync(mlx_ptr)
        return result

    def mlx_sync(
        self, mlx_ptr: Optional[int], cmd: int, img_or_win_ptr: Optional[int]
    ) -> int:
        """Run one of the ``SYNC_*`` commands against an image or a window."""
        self.mlx_func.mlx_sync.argtypes = [
            ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p,
        ]
        self.mlx_func.mlx_sync.restype = ctypes.c_int
        result: int = self.mlx_func.mlx_sync(mlx_ptr, cmd, img_or_win_ptr)
        return result

    #: mlx_sync() command: wait until the image's data can be written again.
    SYNC_IMAGE_WRITABLE = 1
    #: mlx_sync() command: wait until all pending requests are sent.
    SYNC_WIN_FLUSH = 2
    #: mlx_sync() command: wait until pending requests are sent and completed.
    SYNC_WIN_COMPLETED = 3
