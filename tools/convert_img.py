"""
Convert Laughing Man PNG to LVGL C array (RGB565 + Alpha)
- Resize to 240px height (maintain aspect ratio)
- Output as LV_IMG_CF_TRUE_COLOR_ALPHA (RGB565 + 1 byte alpha per pixel)
"""
from PIL import Image
import struct
import sys
import os

def rgb888_to_rgb565(r, g, b):
    """Convert RGB888 to RGB565 (little-endian)"""
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

def convert_image(input_path, output_path, target_height=240, alpha_level=80):
    """
    Convert PNG to LVGL C array with semi-transparency.
    alpha_level: 0-255, the max opacity for the image (80 = ~31% visible)
    """
    img = Image.open(input_path).convert("RGBA")
    
    # Calculate new dimensions (maintain aspect ratio, height = target)
    w, h = img.size
    ratio = target_height / h
    new_w = int(w * ratio)
    new_h = target_height
    
    img = img.resize((new_w, new_h), Image.LANCZOS)
    
    print(f"Resized to: {new_w}x{new_h}")
    
    pixels = img.load()
    
    # Generate C array
    var_name = "laughing_man"
    
    with open(output_path, 'w') as f:
        f.write('#include "lvgl.h"\n\n')
        f.write(f'#ifndef LV_ATTRIBUTE_MEM_ALIGN\n')
        f.write(f'#define LV_ATTRIBUTE_MEM_ALIGN\n')
        f.write(f'#endif\n\n')
        f.write(f'#ifndef LV_ATTRIBUTE_IMG_{var_name.upper()}\n')
        f.write(f'#define LV_ATTRIBUTE_IMG_{var_name.upper()}\n')
        f.write(f'#endif\n\n')
        
        # CF_TRUE_COLOR_ALPHA: 2 bytes RGB565 + 1 byte alpha = 3 bytes per pixel
        total_bytes = new_w * new_h * 3
        
        f.write(f'const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_IMG_{var_name.upper()}\n')
        f.write(f'uint8_t {var_name}_map[] = {{\n')
        
        count = 0
        for y in range(new_h):
            for x in range(new_w):
                r, g, b, a = pixels[x, y]
                # Scale alpha by alpha_level
                a = int(a * alpha_level / 255)
                rgb565 = rgb888_to_rgb565(r, g, b)
                lo = rgb565 & 0xFF
                hi = (rgb565 >> 8) & 0xFF
                
                f.write(f'0x{lo:02x}, 0x{hi:02x}, 0x{a:02x}, ')
                count += 3
                if count % 36 == 0:
                    f.write('\n')
        
        f.write('};\n\n')
        
        f.write(f'const lv_img_dsc_t {var_name} = {{\n')
        f.write(f'    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n')
        f.write(f'    .header.always_zero = 0,\n')
        f.write(f'    .header.reserved = 0,\n')
        f.write(f'    .header.w = {new_w},\n')
        f.write(f'    .header.h = {new_h},\n')
        f.write(f'    .data_size = {total_bytes},\n')
        f.write(f'    .data = {var_name}_map,\n')
        f.write(f'}};\n')
    
    print(f"Output: {output_path}")
    print(f"Data size: {total_bytes} bytes ({total_bytes/1024:.1f} KB)")

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)
    
    input_file = os.path.join(project_dir, "fonts", "laughing_man.png")
    output_file = os.path.join(project_dir, "src", "laughing_man.c")
    
    # alpha_level=80 means ~31% opacity (semi-transparent ghost overlay)
    convert_image(input_file, output_file, target_height=240, alpha_level=80)
