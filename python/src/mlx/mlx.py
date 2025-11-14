# MLX python wrapper for Mlx C library
# See mlx manuals and mlx.h from C library for function usage
# C Functions that require addresses to pass back info are converted to
#  Pythod methods that return a tuple


from ctypes import *
import os

class Mlx:

  def __init__(self):
    module_dir = os.path.dirname(os.path.abspath(__file__))
    self.so_file = os.path.join(module_dir, "libmlx.so")
    self.mlx_func = CDLL(self.so_file)
    self._python_ref_std = {}
    self._python_ref_gen = {}
    self._img_height = {}
    
# Initialisation
  def mlx_init(self):
    self.mlx_func.mlx_init.restype = c_void_p
    return self.mlx_func.mlx_init()

  def mlx_release(self, mlx_ptr):
    self.mlx_func.mlx_release.argtypes = [c_void_p]
    self.mlx_func.mlx_release.restypes = [c_int]
    return self.mlx_func.mlx_release(mlx_ptr)

# Windows
  def mlx_new_window(self, mlx_ptr, width, height, title):
    self.mlx_func.mlx_new_window.argtypes = [c_void_p, c_uint, c_uint, c_char_p]
    self.mlx_func.mlx_new_window.restype = c_void_p
    return self.mlx_func.mlx_new_window(mlx_ptr, width, height, title.encode('utf-8'))

  def mlx_clear_window(self, mlx_ptr, win_ptr):
    self.mlx_func.mlx_clear_window.argtypes = [c_void_p, c_void_p]
    self.mlx_func.mlx_clear_window.restype = c_int
    return self.mlx_func.mlx_clear_window(mlx_ptr, win_ptr)

  def mlx_pixel_put(self, mlx_ptr, win_ptr, x, y, color):
    self.mlx_func.mlx_pixel_put.argtypes = [c_void_p, c_void_p, c_uint, c_uint, c_uint]
    self.mlx_func.mlx_pixel_put.restype = c_int
    return self.mlx_func.mlx_pixel_put(mlx_ptr, win_ptr, x, y, color)

  def mlx_destroy_window(self, mlx_ptr, win_ptr):
    self.mlx_func.mlx_destroy_window.argtypes = [c_void_p, c_void_p]
    self.mlx_func.mlx_destroy_window.restype = c_int
    return self.mlx_func.mlx_destroy_window(mlx_ptr, win_ptr)

# Images
  def mlx_new_image(self, mlx_ptr, width, height):
    self.mlx_func.mlx_new_image.argtypes = [c_void_p, c_uint, c_uint]
    self.mlx_func.mlx_new_image.restype = c_void_p
    ret = self.mlx_func.mlx_new_image(mlx_ptr, width, height)
    if ret is not None:
      self._img_height[str(ret)] = height
    return ret

# API break, returns tuple
  def mlx_get_data_addr(self, img_ptr):
    bits_per_pixel = c_uint()
    size_line = c_uint()
    theformat = c_uint()
    data = POINTER(c_char)
    self.mlx_func.mlx_get_data_addr.argtypes = [c_void_p, POINTER(c_uint), POINTER(c_uint), POINTER(c_uint)]
    self.mlx_func.mlx_get_data_addr.restype = POINTER(c_char)
    data = self.mlx_func.mlx_get_data_addr(img_ptr, byref(bits_per_pixel), byref(size_line), byref(theformat))
    data_array = c_char * (self._img_height[str(img_ptr)] * size_line.value)
    data_view = data_array.from_address(addressof(data.contents))
    return (memoryview(data_view).cast('B'), bits_per_pixel.value, size_line.value, theformat.value)

  def mlx_put_image_to_window(self, mlx_ptr, win_ptr, img_ptr, x, y):
    self.mlx_func.mlx_put_image_to_window.argtypes = [c_void_p, c_void_p, c_void_p, c_int, c_int]
    self.mlx_func.mlx_put_image_to_window.restype = c_int
    return self.mlx_func.mlx_put_image_to_window(mlx_ptr, win_ptr, img_ptr, x, y)

  def mlx_destroy_image(self, mlx_ptr, img_ptr):
    self._img_height.pop(str(img_ptr))
    self.mlx_func.mlx_destroy_image.argtypes = [c_void_p, c_void_p]
    self.mlx_func.mlx_destroy_image.restype = c_int
    return self.mlx_func.mlx_destroy_image(mlx_ptr, img_ptr)

# Events & main loop
# Note: Python can't catch C^-C from keyboard during mlx_loop execution.
#  Use C^-\ to kill your program.

  def mlx_loop(self, mlx_ptr):
    self.mlx_func.mlx_loop.argtypes = [c_void_p]
    self.mlx_func.mlx_loop.restype = c_int
    return self.mlx_func.mlx_loop(mlx_ptr)

  def mlx_loop_exit(self, mlx_ptr):
    self.mlx_func.mlx_loop_exit.argtypes = [c_void_p]
    self.mlx_func.mlx_loop_exit.restype = c_int
    return self.mlx_func.mlx_loop_exit(mlx_ptr)

  def mlx_mouse_hook(self, win_ptr, callback, param):
    self.mlx_func.mlx_mouse_hook.restype = c_int
    if not callback:
      self._python_ref_std[str(win_ptr)+"_mouse_f"] = None
      self._python_ref_std[str(win_ptr)+"_mouse_p"] = None
      self.mlx_func.mlx_mouse_hook.argtypes = [c_void_p, c_void_p, c_void_p]
      return self.mlx_func.mlx_mouse_hook(win_ptr, None, None)
    callback_type = CFUNCTYPE(None, c_uint, c_uint, c_uint, py_object)
    self.mlx_func.mlx_mouse_hook.argtypes = [c_void_p, callback_type, py_object]
    callback_ref = callback_type(callback)
    self._python_ref_std[str(win_ptr)+"_mouse_f"] = callback_ref
    self._python_ref_std[str(win_ptr)+"_mouse_p"] = param
    return self.mlx_func.mlx_mouse_hook(win_ptr, callback_ref, param)

  def mlx_key_hook(self, win_ptr, callback, param):
    self.mlx_func.mlx_key_hook.restype = c_int
    if not callback:
      self._python_ref_std[str(win_ptr)+"_key_f"] = None
      self._python_ref_std[str(win_ptr)+"_key_p"] = None
      self.mlx_func.mlx_key_hook.argtypes = [c_void_p, c_void_p, c_void_p]
      return self.mlx_func.mlx_key_hook(win_ptr, None, None)
    callback_type = CFUNCTYPE(None, c_uint, py_object)
    self.mlx_func.mlx_key_hook.argtypes = [c_void_p, callback_type, py_object]
    callback_ref = callback_type(callback)
    self._python_ref_std[str(win_ptr)+"_key_f"] = callback_ref
    self._python_ref_std[str(win_ptr)+"_key_p"] = param
    return self.mlx_func.mlx_key_hook(win_ptr, callback_ref, param)

  def mlx_expose_hook(self, win_ptr, callback, param):
    self.mlx_func.mlx_expose_hook.restype = c_int
    if not callback:
      self._python_ref_std[str(win_ptr)+"_expose_f"] = None
      self._python_ref_std[str(win_ptr)+"_expose_p"] = None
      self.mlx_func.mlx_expose_hook.argtypes = [c_void_p, c_void_p, c_void_p]
      return self.mlx_func.mlx_expose_hook(win_ptr, None, None)
    callback_type = CFUNCTYPE(None, py_object)
    self.mlx_func.mlx_expose_hook.argtypes = [c_void_p, callback_type, py_object]
    callback_ref = callback_type(callback)
    self._python_ref_std[str(win_ptr)+"_expose_f"] = callback_ref
    self._python_ref_std[str(win_ptr)+"_expose_p"] = param
    return self.mlx_func.mlx_expose_hook(win_ptr, callback_ref, param)

  def mlx_loop_hook(self, mlx_ptr, callback, param):
    self.mlx_func.mlx_loop_hook.restype = c_int
    if not callback:
      self._python_ref_std["loop_f"] = None
      self._python_ref_std["loop_p"] = None
      self.mlx_func.mlx_loop_hook.argtypes = [c_void_p, c_void_p, c_void_p]
      return self.mlx_func.mlx_loop_hook(mlx_ptr, None, None)
    callback_type = CFUNCTYPE(None, py_object)
    self.mlx_func.mlx_loop_hook.argtypes = [c_void_p, callback_type, py_object]
    callback_ref = callback_type(callback)
    self._python_ref_std["loop_f"] = callback_ref
    self._python_ref_std["loop_p"] = param
    return self.mlx_func.mlx_loop_hook(mlx_ptr, callback_ref, param)

  def mlx_hook(self, win_ptr, x_event, x_mask, callback, param):
    x_event_key = [2, 3]
    x_event_mouse = [4, 5]
    x_event_motion = [6]
    self.mlx_func.mlx_hook.restype = c_int
    if not callback:
      self._python_ref_gen[str(win_ptr)+"_f_"+str(x_event)] = None
      self._python_ref_gen[str(win_ptr)+"_p_"+str(x_event)] = None
      self.mlx_func.mlx_hook.argtypes = [c_void_p, c_uint, c_uint, c_void_p, c_void_p]
      return self.mlx_func.mlx_hook(win_ptr, 0, 0, None, None)
    if x_event in x_event_key:
      callback_type = CFUNCTYPE(None, c_uint, py_object)
    elif x_event in x_event_mouse:
      callback_type = CFUNCTYPE(None, c_uint, c_uint, c_uint, py_object)
    elif x_event in x_event_motion:      
      callback_type = CFUNCTYPE(None, c_uint, c_uint, py_object)
    else:
      callback_type = CFUNCTYPE(None, py_object)

    self.mlx_func.mlx_hook.argtypes = [c_void_p, c_uint, c_uint, callback_type, py_object]
    callback_ref = callback_type(callback)
    self._python_ref_gen[str(win_ptr)+"_f_"+str(x_event)] = callback_ref
    self._python_ref_gen[str(win_ptr)+"_p_"+str(x_event)] = param
    return self.mlx_func.mlx_hook(win_ptr, x_event, x_mask, callback_ref, param)

# Misc.

  def mlx_string_put(self, mlx_ptr, win_ptr, x, y, color, string):
    self.mlx_func.mlx_string_put.argtypes = [c_void_p, c_void_p, c_uint, c_uint, c_uint, c_char_p]
    self.mlx_func.mlx_string_put.restype = c_int
    return self.mlx_func.mlx_string_put(mlx_ptr, win_ptr, x, y, color, string.encode('utf-8'))

# API break, returns tuple
  def mlx_xpm_file_to_image(self, mlx_ptr, filename):
    width = c_uint()
    height = c_uint()
    self.mlx_func.mlx_xpm_file_to_image.argtypes = [c_void_p, c_char_p, c_void_p, c_void_p]
    self.mlx_func.mlx_xpm_file_to_image.restype = c_void_p
    img = self.mlx_func.mlx_xpm_file_to_image(mlx_ptr, filename.encode('utf8'), byref(width), byref(height))
    if img is not None:
      self._img_height[str(img)] = height.value
    return (img, width.value, height.value)

# API break, returns tuple
  def mlx_png_file_to_image(self, mlx_ptr, filename):
    width = c_uint()
    height = c_uint()
    self.mlx_func.mlx_png_file_to_image.argtypes = [c_void_p, c_char_p, c_void_p, c_void_p]
    self.mlx_func.mlx_png_file_to_image.restype = c_void_p
    img = self.mlx_func.mlx_png_file_to_image(mlx_ptr, filename.encode('utf8'), byref(width), byref(height))
    if img is not None:
      self._img_height[str(img)] = height.value
    return (img, width.value, height.value)

# not really usefull in Python context    
#void	*mlx_xpm_to_image(void *mlx_ptr, const char **xpm_data,
#			  unsigned int *width, unsigned int *height);

  def mlx_mouse_hide(self, mlx_ptr):
    self.mlx_func.mlx_mouse_hide.argtypes = [c_void_p]
    self.mlx_func.mlx_mouse_hide.restype = c_int
    return self.mlx_func.mlx_mouse_hide(mlx_ptr)

  def mlx_mouse_show(self, mlx_ptr):
    self.mlx_func.mlx_mouse_show.argtypes = [c_void_p]
    self.mlx_func.mlx_mouse_show.restype = c_int
    return self.mlx_func.mlx_mouse_show(mlx_ptr)

  def mlx_mouse_move(self, mlx_ptr, x, y):
    self.mlx_func.mlx_mouse_move.argtypes = [c_void_p, c_int, c_int]
    self.mlx_func.mlx_mouse_move.restype = c_int
    return self.mlx_func.mlx_mouse_move(mlx_ptr, x, y)

# API break, returns tuple
  def mlx_mouse_get_pos(self, mlx_ptr):
    x = c_int()
    y = c_int()
    self.mlx_func.mlx_mouse_get_pos.argtypes = [c_void_p, c_void_p, c_void_p]
    self.mlx_func.mlx_mouse_get_pos.restype = c_int
    val = self.mlx_func.mlx_mouse_get_pos(mlx_ptr, byref(x), byref(y))
    return (val, x.value, y.value)

  def mlx_do_key_autorepeatoff(self, mlx_ptr):
    self.mlx_func.mlx_do_key_autorepeatoff.argtypes = [c_void_p]
    self.mlx_func.mlx_do_key_autorepeatoff.restype = c_int
    return self.mlx_func.mlx_do_key_autorepeatoff(mlx_ptr)

  def mlx_do_key_autorepeaton(self, mlx_ptr):
    self.mlx_func.mlx_do_key_autorepeaton.argtypes = [c_void_p]
    self.mlx_func.mlx_do_key_autorepeaton.restype = c_int
    return self.mlx_func.mlx_do_key_autorepeaton(mlx_ptr)

# API break, returns tuple
  def mlx_get_screen_size(self, mlx_ptr):
    w = c_uint()
    h = c_uint()
    self.mlx_func.mlx_get_screen_size.argtypes = [c_void_p, POINTER(c_uint), POINTER(c_uint)]
    self.mlx_func.mlx_get_screen_size.restype = c_int
    val = self.mlx_func.mlx_get_screen_size(mlx_ptr, byref(w), byref(h))
    return (val, w.value, h.value)

# Sync funct
  def mlx_do_sync(self, mlx_ptr):
    self.mlx_func.mlx_do_sync.argtypes = [c_void_p]
    self.mlx_func.mlx_do_sync.restype = c_int
    return self.mlx_func.mlx_do_sync(mlx_ptr)

  def mlx_sync(self, mlx_ptr, cmd, img_or_win_ptr):
    self.mlx_func.mlx_sync.argtypes = [c_void_p, c_int, c_void_p]
    self.mlx_func.mlx_sync.restype = c_int
    return self.mlx_func.mlx_sync(mlx_ptr, cmd, img_or_win_ptr)

  SYNC_IMAGE_WRITABLE = 1
  SYNC_WIN_FLUSH = 2
  SYNC_WIN_COMPLETED = 3
