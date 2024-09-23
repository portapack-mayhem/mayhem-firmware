import os
import re
from fontTools.ttLib import TTFont

def get_longest_copyright(font):
    decodable_copyrights = []
    for name in font['name'].names:
        try:
            decoded_string = name.string.decode('utf-8')
            if decoded_string:
                decodable_copyrights.append(decoded_string)
        except UnicodeDecodeError:
            pass
    if decodable_copyrights:
        return max(decodable_copyrights, key=len)
    else:
        return None

def read_copyright_info(font_file):
    try:
        font = TTFont(font_file)
        copyright_info = get_longest_copyright(font)

        #decode copyright info, it has a lot of \x00, replace all those for nothing
        
        if copyright_info:
            return re.sub(r'\x00', '', copyright_info)
        else:
            print(f"Could not find any copyright info in {font_file}")
            return None
        
    except Exception as e:
        print(f"Error reading copyright info from {font_file}: {e}")
        return None

def write_license_file(font_file, copyright_info):
    try:
        base_name, ext = os.path.splitext(font_file)
        license_file = f"{base_name}-info.txt"
        with open(license_file, 'w') as f:
            f.write(copyright_info)
        print(f"License file written: {license_file}")
    except Exception as e:
        print(f"Error writing license file for {font_file}: {e}")

def main():
    folder_path = os.path.dirname(__file__)
    for filename in os.listdir(folder_path):
        if filename.endswith('.ttf') or filename.endswith('.otf'):
            font_file = os.path.join(folder_path, filename)
            copyright_info = read_copyright_info(font_file)
            if copyright_info:
                write_license_file(font_file, copyright_info)

if __name__ == "__main__":
    main()
