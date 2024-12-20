bitmap is for icons, it's colorless and headless; splash and modal isn't 
using bitmap  

## Bitmap helper folder
The folder `bitmap_tools` contains scripts to convert icons in png format to 
the PortaPack readable format in `bitmap.hpp`.  

### Convert a folder contains one or more icon.png to one bitmap.hpp
The `make_bitmap.py` is the traditional helper, well tested. The folder with 
the icons is given as argument and generates the `./bitmap.hpp`. This file 
needs to be copied to `mayhem-firmware/firmware/application/`.  
The icon size needs to be a multiple of 8. The generated icon is black/white.  

### Convert bitmap array to icon.png
The `bitmap_arr_reverse_decode.py` takes an array from the bitmap.hpp and 
convert it back to a png.  

### Convert both ways
The `pp_png2hpp.py` is based on the privious scripts, as all in one solution.  
With the `--hpp bitmap.hpp` file and the `--graphics /folder_to/png_icons/` 
arguments, theis script will generate a `bitmap.hpp`.  
Add the `--reverse` argument to generate png icons from the given `bitmap.hpp`.  

Especially the reverse function got a parser, to automatic get the filename 
and size of the image.