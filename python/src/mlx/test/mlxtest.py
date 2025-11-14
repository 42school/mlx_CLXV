# Mlx large test

import sys
from mlx import Mlx  # Import Mlx class

class ImgData:
    """Structure for image data"""
    def __init__(self):
        self.img = None
        self.width = 0
        self.height = 0
        self.data = None
        self.sl = 0  # size line
        self.bpp = 0  # bits per pixel
        self.iformat = 0

class XVar:
    """Structure for main vars"""
    def __init__(self):
        self.mlx = None
        self.mlx_ptr = None
        self.screen_w = 0
        self.screen_h = 0
        self.win_1 = None
        self.win_2 = None
        self.img_1 = ImgData()
        self.img_2 = ImgData()
        self.img_png = ImgData()
        self.img_xpm = ImgData()
        self.imgidx = 0

def draw_colormap(xvar):
    """Draw the colormap"""
    print("Drawing colormap...")
    for i in range(400):
        for j in range(400):
            r = int((0xFF * i) / 400)
            g = int((0xFF * j) / 400)
            b = int((0xFF * (400 - (i + j) // 2)) / 400)
            col = 0xFF000000 | (r << 16) | (g << 8) | b
            xvar.mlx.mlx_pixel_put(xvar.mlx_ptr, xvar.win_1, i, j, col)

def gere_key_press(key, xvar):
    print(f"Pressed key {key}")

def gere_key(key, xvar):
    print(f"Got key {key}: ", end="")
    
    if key == 113:  # 'q'
        xvar.mlx.mlx_do_key_autorepeatoff(xvar.mlx_ptr)
        print("key repeat off")
        return 0
    elif key == 119:  # 'w'
        xvar.mlx.mlx_do_key_autorepeaton(xvar.mlx_ptr)
        print("key repeat on")
        return 0
    elif key == 101:  # 'e'
        draw_colormap(xvar)
        print("colormap")
        return 0
    elif key == 114:  # 'r'
        xvar.mlx.mlx_mouse_hide(xvar.mlx_ptr)
        print("mouse hide")
        return 0
    elif key == 116:  # 't'
        xvar.mlx.mlx_mouse_show(xvar.mlx_ptr)
        print("mouse show")
        return 0
    elif key == 121:  # 'y'
        xvar.mlx.mlx_mouse_move(xvar.win_1, 200, 200)
        print("mouse move")
        return 0
    elif key == 117:  # 'u'
        ret, x, y = xvar.mlx.mlx_mouse_get_pos(xvar.win_1)
        print(f"current mouse pos is {x} x {y}")
        return 0
    elif key == 105:  # 'i'
        xvar.mlx.mlx_sync(xvar.mlx_ptr, Mlx.SYNC_IMAGE_WRITABLE, xvar.img_2.img)
        # fill image in white
        for offset in range(0, xvar.img_2.sl * 100, 4):
            xvar.img_2.data[offset:offset+4] = (0xFFFFFFFF).to_bytes(4, 'little')
        
        xvar.mlx.mlx_put_image_to_window(xvar.mlx_ptr, xvar.win_1, xvar.img_2.img, 50, 50)
        
        # update in red
        for offset in range(0, xvar.img_2.sl * 100, 4):
            xvar.img_2.data[offset:offset+4] = (0xFFFF0000).to_bytes(4, 'little')
        
        xvar.mlx.mlx_put_image_to_window(xvar.mlx_ptr, xvar.win_1, xvar.img_2.img, 250, 250)
        print("update image without sync - most likely 2 red squares")
        return 0
    elif key == 111:  # 'o'
        xvar.mlx.mlx_sync(xvar.mlx_ptr, Mlx.SYNC_IMAGE_WRITABLE, xvar.img_2.img)
        # fill image in white
        for offset in range(0, xvar.img_2.sl * 100, 4):
            xvar.img_2.data[offset:offset+4] = (0xFFFFFFFF).to_bytes(4, 'little')
        
        xvar.mlx.mlx_put_image_to_window(xvar.mlx_ptr, xvar.win_1, xvar.img_2.img, 50, 50)
        xvar.mlx.mlx_sync(xvar.mlx_ptr, Mlx.SYNC_IMAGE_WRITABLE, xvar.img_2.img)
        
        # update in red
        for offset in range(0, xvar.img_2.sl * 100, 4):
            xvar.img_2.data[offset:offset+4] = (0xFFFF0000).to_bytes(4, 'little')
        
        xvar.mlx.mlx_put_image_to_window(xvar.mlx_ptr, xvar.win_1, xvar.img_2.img, 250, 250)
        print("update image with sync - white and red squares")
        return 0
    
    # Default
    print("clear and string put")
    xvar.mlx.mlx_clear_window(xvar.mlx_ptr, xvar.win_1)
    xvar.mlx.mlx_string_put(xvar.mlx_ptr, xvar.win_1, 20, 20, 0xFFFF00FF, "Hello MLX!")


def gere_expose(xvar):
    print("Expose !")
    xvar.mlx.mlx_put_image_to_window(xvar.mlx_ptr, xvar.win_1, xvar.img_1.img, 0, 0)
    xvar.mlx.mlx_put_image_to_window(xvar.mlx_ptr, xvar.win_1, xvar.img_1.img, 201, 201)

def gere_mouse(button, x, y, xvar, win):
    print(f"Got mouse : {button} at {x}x{y}")
    
    if button == 1:
        xvar.mlx.mlx_put_image_to_window(xvar.mlx_ptr, win, xvar.img_1.img, 100, 100)
        return 0
    
    if button == 3:  # right click
        if xvar.imgidx % 2:
            xvar.mlx.mlx_put_image_to_window(xvar.mlx_ptr, win, xvar.img_png.img, x, y)
        else:
            xvar.mlx.mlx_put_image_to_window(xvar.mlx_ptr, win, xvar.img_xpm.img, x, y)
        xvar.imgidx += 1

def gere_mouse_1(button, x, y, xvar):
    gere_mouse(button, x, y, xvar, xvar.win_1)

def gere_mouse_2(button, x, y, xvar):
    gere_mouse(button, x, y, xvar, xvar.win_2)

def gere_close_1(xvar):
    xvar.mlx.mlx_loop_exit(xvar.mlx_ptr)

def gere_close_2(xvar):
    xvar.mlx.mlx_destroy_window(xvar.mlx_ptr, xvar.win_2)
    xvar.win_2 = None

def main():
    xvar = XVar()
    # Mlx Initialisation
    try:
        xvar.mlx = Mlx()
    except Exception as e:
        print(f"Error: Can't initialize MLX: {e}", file=sys.stderr)
        sys.exit(1)

    xvar.mlx_ptr = xvar.mlx.mlx_init()
    
    ret, xvar.screen_w, xvar.screen_h = xvar.mlx.mlx_get_screen_size(xvar.mlx_ptr)
    print(f"Screen size: {xvar.screen_w} x {xvar.screen_h}")
    
    # Windows creation
    try:
        xvar.win_1 = xvar.mlx.mlx_new_window(xvar.mlx_ptr, 400, 400, "MLX main win")
        if not xvar.win_1:
            raise Exception("Can't create main window")
            
        xvar.win_2 = xvar.mlx.mlx_new_window(xvar.mlx_ptr, 150, 150, "Secondary window")
        if not xvar.win_2:
            raise Exception("Can't create secondary window")
    except Exception as e:
        print(f"Error Win create: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Image #1
    xvar.img_1.img = xvar.mlx.mlx_new_image(xvar.mlx_ptr, 200, 200)
    if not xvar.img_1.img:
        raise Exception("Can't create image 1")
            
    xvar.img_1.width = 200
    xvar.img_1.height = 200
    xvar.img_1.data, xvar.img_1.bpp, xvar.img_1.sl, xvar.img_1.iformat = \
        xvar.mlx.mlx_get_data_addr(xvar.img_1.img)

    # Fill image #1
    for i in range(xvar.img_1.sl * 200):
        xvar.img_1.data[i] = 0x80

    for i in range(xvar.img_1.sl * 100):
        xvar.img_1.data[i] = 0xFF

    try:
        # Add some red pixels
        pixel_positions = [
            0 * 200 * 4,                   # top left
            (1 * 200 + 1) * 4,             # top left + 1
            (199 * 200 + 199) * 4,         # bottom right
            (198 * 200 + 198) * 4          # bottom right - 1
        ]
        
        for pos in pixel_positions:
            if pos < len(xvar.img_1.data) - 3:
                xvar.img_1.data[pos:pos+4] = (0xFFFF0000).to_bytes(4, 'little')
    except Exception as e:
        print(f"Error img1: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Image #2
    try:
        xvar.img_2.img = xvar.mlx.mlx_new_image(xvar.mlx_ptr, 100, 100)
        if not xvar.img_2.img:
            raise Exception("Can't create image 2")
            
        xvar.img_2.width = 100
        xvar.img_2.height = 100
        xvar.img_2.data, xvar.img_2.bpp, xvar.img_2.sl, xvar.img_2.iformat = \
            xvar.mlx.mlx_get_data_addr(xvar.img_2.img)
    except Exception as e:
        print(f"Error img2: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Load PNG & XPM
    result = xvar.mlx.mlx_png_file_to_image(xvar.mlx_ptr, "puffy_small.png")
    if not result:
        raise Exception("Can't load PNG")
    xvar.img_png.img, xvar.img_png.width, xvar.img_png.height = result
    if not xvar.img_png.img:
        raise Exception("Can't create png")
    xvar.img_png.data, xvar.img_png.bpp, xvar.img_png.sl, xvar.img_png.iformat = \
        xvar.mlx.mlx_get_data_addr(xvar.img_png.img)
        
    result = xvar.mlx.mlx_xpm_file_to_image(xvar.mlx_ptr, "Dont_panic.xpm")
    if not result:
        raise Exception("Can't load XPM")
    xvar.img_xpm.img, xvar.img_xpm.width, xvar.img_xpm.height = result
    xvar.img_xpm.data, xvar.img_xpm.bpp, xvar.img_xpm.sl, xvar.img_xpm.iformat = \
        xvar.mlx.mlx_get_data_addr(xvar.img_xpm.img)
    
    # event hooks
    xvar.mlx.mlx_key_hook(xvar.win_1, gere_key, xvar)
    xvar.mlx.mlx_hook(xvar.win_2, 2, 1, gere_key_press, xvar)  # KeyPress event
    xvar.mlx.mlx_expose_hook(xvar.win_1, gere_expose, xvar)
    xvar.mlx.mlx_mouse_hook(xvar.win_1, gere_mouse_1, xvar)
    xvar.mlx.mlx_mouse_hook(xvar.win_2, gere_mouse_2, xvar)
    xvar.mlx.mlx_hook(xvar.win_1, 33, 0, gere_close_1, xvar)  # WM_DELETE_WINDOW
    xvar.mlx.mlx_hook(xvar.win_2, 33, 0, gere_close_2, xvar)  # WM_DELETE_WINDOW
    
    # User Instructions
    print("On main window:")
    print(" mouse button 1: place white/gray image in 0x0 and 200x200")
    print(" mouse button 2: place png image and xpm image, alternatively")
    print(" try keys QWERTYUIO and others")
    print(" click window's X button to end the program")
    print("On secondary window (smaller):")
    print(" show key pressed for auto repeat")
    print(" click window's X button to close it")
    
    # Main loop
    xvar.mlx.mlx_loop(xvar.mlx_ptr)
    
    # Cleaning resources
    print("destroy xpm")
    xvar.mlx.mlx_destroy_image(xvar.mlx_ptr, xvar.img_xpm.img)
    print("destroy png")
    xvar.mlx.mlx_destroy_image(xvar.mlx_ptr, xvar.img_png.img)
    print("destroy imgs")
    xvar.mlx.mlx_destroy_image(xvar.mlx_ptr, xvar.img_1.img)
    xvar.mlx.mlx_destroy_image(xvar.mlx_ptr, xvar.img_2.img)
    print("destroy win(s)")
    xvar.mlx.mlx_destroy_window(xvar.mlx_ptr, xvar.win_1)
    if xvar.win_2:
        xvar.mlx.mlx_destroy_window(xvar.mlx_ptr, xvar.win_2)
    print("destroy mlx")
    xvar.mlx.mlx_release(xvar.mlx_ptr)

if __name__ == "__main__":
    main()
