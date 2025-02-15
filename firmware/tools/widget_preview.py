#
# copyleft Elliot Alderson from F society
# copyleft Darlene Alderson from F society
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#
import re
import tkinter as tk
from typing import Dict, List, Tuple
import argparse
from dataclasses import dataclass

"""
TODO: Multile class in one file seperation
TODO: Add more widget type

widget compatible guide:

1. add the preview color in the "widgets = {" table 
(note that this color is only for easier to distinguish)

2. add the widget type and also regex in the "parsers" table
(note that your regex should apply all the possible constructor overload (re-impl))

"""

# pp hard coded vars
screen_width = 240
SCREEN_W = 240
screen_height = 320
CHARACTER_WIDTH = 8
LINE_HEIGHT = 16
topbar_offset = 16 
# widgets actually drew after the top bar,
# for example if Y = 0 then Y on screen actually is 16

# scale factor
scale = 2

widgets = {
    "Button": "lightgray",
    "NewButton": "lightyellow", 
    "Text": "lightblue",
    "Rectangle": "lightgreen",
    "Image": "pink",
    "ImageButton": "lightpink",
    "NumberField": "peachpuff",
    "ProgressBar": "lightcoral",
    "Console": "wheat",
    "Checkbox": "plum",
    "Label": "lavender",
    "TextField": "paleturquoise",
    "OptionsField": "palegreen",
    "VuMeter": "sandybrown",
    "BigFrequency": "khaki"
}

@dataclass
class Widget:
    name: str
    widget_type: str
    x: int
    y: int
    width: int
    height: int
    text: str = ""

class WidgetParser:
    def __init__(self):

        self.parsers: Dict[str, re.Pattern] = {
            'Button': re.compile( # TODO: fix those that using language helper
                r'Button\s+(\w+)\s*\{\s*\{([^}]+)\},\s*"([^"]+)"\s*\};',
                re.MULTILINE
            ),
            'NewButton': re.compile(
                r'NewButton\s+(\w+)\s*\{\s*(?:\{([^}]+)\}|{}),\s*(?:"([^"]*)"|\{\}),\s*(?:[^,}]+(?:,\s*[^}]+)*|\{\})\};',
                re.MULTILINE
            ),
            'Text': re.compile(
                r'Text\s+(\w+)\s*\{\s*\{([^}]+)\}(?:\s*,\s*"([^"]*)")?\s*\};',
                re.MULTILINE
            ),
            'ProgressBar': re.compile(
                r'ProgressBar\s+(\w+)\s*\{\s*\{([^}]+)\}\s*\};',
                re.MULTILINE
            ),
            'Console': re.compile(
                r'Console\s+(\w+)\s*\{\s*\{([^}]+)\}\s*\};',
                re.MULTILINE
            ),
            'Label': re.compile(
                r'Label\s+(\w+)\s*\{\s*\{([^}]+)\},\s*"([^"]+)"\s*\};',
                re.MULTILINE
            ),
            'VuMeter': re.compile(
                r'VuMeter\s+(\w+)\s*\{\s*\{([^}]+)\},\s*\d+,\s*(?:true|false)\s*\};',
                re.MULTILINE
            ),
            'BigFrequency': re.compile(
                r'BigFrequency\s+(\w+)\s*\{\s*\{([^}]+)\},\s*\d+\s*\};',
                re.MULTILINE
            )
        }
    
    def parse_coordinates(self, coord_str: str) -> Tuple[int, int, int, int]:
        """Parse coordinate string like '2 * 8, 8 * 16, 8 * 8, 3 * 16'"""
        if not coord_str or coord_str.isspace():
            return (0, 0, 0, 0)  # for empty constructor fallback
        
        # split and evaluate each coordinate expression
        coords = [eval(x.strip()) for x in coord_str.split(',')]
        
        # ensure we have exactly 4 coordinates, pad with zeros if needed
        while len(coords) < 4:
            coords.append(0)
        
        # only take the first 4 coordinates if there are more
        return tuple(coords[:4])

    def parse_file(self, filepath: str) -> List[Widget]:
        with open(filepath, 'r') as f:
            content = f.read()
        
        widgets = []
        for widget_type, pattern in self.parsers.items():
            matches = pattern.finditer(content)
            for match in matches:
                name = match.group(1)  # Widget name is always in group 1
                coords = self.parse_coordinates(match.group(2) if match.group(2) else "")
                
                # Only try to get text if the pattern has 3 or more groups
                text = ""
                try:
                    if match.lastindex >= 3:
                        text = match.group(3) if match.group(3) else ""
                except IndexError:
                    pass
                
                widgets.append(Widget(
                    name=name,
                    widget_type=widget_type,
                    x=coords[0],
                    y=coords[1],
                    width=coords[2],
                    height=coords[3],
                    text=text
                ))
        
        return widgets

class WidgetPreview(tk.Tk):
    def __init__(self, widgets: List[Widget]):
        super().__init__()
        self.title("Widget Preview")
        
        self.canvas = tk.Canvas(self, width=screen_width * scale, height=screen_height * scale, bg='white')
        self.canvas.pack(padx=10, pady=10)
        
        self.all_text_elements = []
        self.draw_widgets(widgets)

    def draw_widgets(self, widgets: List[Widget]):
        draw_top_bar(self)
        for widget in widgets:
            self.draw_widget(widget)
    
    def draw_widget(self, widget: Widget):
        print(f"Drawing widget: {widget.name} ({widget.widget_type})")
        
        x1 = widget.x * scale
        y1 = (widget.y + topbar_offset) * scale 
        x2 = x1 + (widget.width * scale)
        y2 = y1 + (widget.height * scale)

        print(f"Coordinates: ({x1}, {y1}), ({x2}, {y2})")
        
        if widget.widget_type == "VuMeter":
            segment_height = widget.height / 8 * scale
            for i in range(8):
                self.canvas.create_rectangle(
                    x1, y1 + (i * segment_height),
                    x2, y1 + ((i+1) * segment_height),
                    fill=widgets[widget.widget_type],
                    outline="gray"
                )
        elif widget.widget_type == "BigFrequency":
            # Draw 7-segment style display
            self.canvas.create_rectangle(
                x1, y1, x2, y2,
                fill=widgets[widget.widget_type],
                outline="gray" 
            )
            self.canvas.create_text(
                (x1 + x2) / 2,
                (y1 + y2) / 2,
                text="433.92" # placeholder text
            )
            
        else:
            # defualt rendering
            rect_id = self.canvas.create_rectangle(
                x1, y1, x2, y2, 
                fill=widgets[widget.widget_type]
            )
            
            type_text_id = self.canvas.create_text(
                (x1 + x2) // 2,
                (y1 + y2) // 2,
                text=widget.widget_type
            )
            
            detail_text_id = self.canvas.create_text(
                (x1 + x2) // 2,
                (y1 + y2) // 2,
                text=f"{widget.widget_type}|{widget.name}|{widget.text}",
                state='hidden'
            )

            widget_texts = {
                'type': type_text_id,
                'detail': detail_text_id
            }
            self.all_text_elements.append(widget_texts)

            # hover handlers
            def on_enter(event):
                for texts in self.all_text_elements:
                    self.canvas.itemconfig(texts['type'], state='hidden')
                    self.canvas.itemconfig(texts['detail'], state='hidden')
                self.canvas.itemconfig(detail_text_id, state='normal')
                self.canvas.tag_raise(detail_text_id)

            def on_leave(event): 
                for texts in self.all_text_elements:
                    self.canvas.itemconfig(texts['type'], state='normal')
                    self.canvas.tag_raise(texts['type'])
                    self.canvas.itemconfig(texts['detail'], state='hidden')

            for item_id in [rect_id, type_text_id, detail_text_id]:
                self.canvas.tag_bind(item_id, '<Enter>', on_enter)
                self.canvas.tag_bind(item_id, '<Leave>', on_leave)

def draw_top_bar(self):
    self.canvas.create_rectangle(0, 0, screen_width * scale, topbar_offset * scale, fill='lightblue')
    self.canvas.create_text(screen_width * scale // 2, topbar_offset * scale // 2, text='I\'m Top Bar, hover mouse on items to check details', fill='black')

def main():
    parser = argparse.ArgumentParser(description='Preview UI widgets from hpp files')
    parser.add_argument('file', help='Path to the hpp file')
    args = parser.parse_args()
    
    widget_parser = WidgetParser()
    widgets = widget_parser.parse_file(args.file)
    
    app = WidgetPreview(widgets)
    app.mainloop()

if __name__ == "__main__":
    main()