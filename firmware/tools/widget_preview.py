import re
import tkinter as tk
from typing import Dict, List, Tuple
import argparse
from dataclasses import dataclass

"""
widget compatible guide:

1. add the preview color in the "widgets = {" table 
(note that this color is only for easier to distinguish)

2. add the widget type and also regex in the "parsers" table
(note that your regex should apply all the possible constructor overload (re-impl))

3. add the widget drawing logic in the "draw_widget" function

"""

# pp hard coded vars
screen_width = 240
screen_height = 320
topbar_offset = 16 
# widgets actually drew after the top bar,
# for example if Y = 0 then Y on screen actually is 16

# scale factor
scale = 3

widgets = {
    "Button": "lightgray",
    "NewButton": "lightyellow",
    "Label": "lightblue",}

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
            'Button': re.compile(
                r'Button\s+(\w+)\s*\{\s*\{([^}]+)\},\s*"([^"]+)"\s*\};',
                re.MULTILINE
            ),
            'NewButton': re.compile(
                r'NewButton\s+(\w+)\s*\{\s*(?:\{([^}]+)\}|{}),\s*(?:"([^"]*)"|\{\}),\s*(?:[^,}]+(?:,\s*[^}]+)*|\{\})\};',
                re.MULTILINE
            ),
        }
    
    def parse_coordinates(self, coord_str: str) -> Tuple[int, int, int, int]:
        """Parse coordinate string like '2 * 8, 8 * 16, 8 * 8, 3 * 16'"""
        if not coord_str or coord_str.isspace():
            return (0, 0, 0, 0)  # for empty constructor fallback
        coords = [eval(x.strip()) for x in coord_str.split(',')]
        return tuple(coords)

    def parse_file(self, filepath: str) -> List[Widget]:
        with open(filepath, 'r') as f:
            content = f.read()
        
        widgets = []
        for widget_type, pattern in self.parsers.items():
            matches = pattern.finditer(content)
            for match in matches:
                name = match.group(1)
                coords = self.parse_coordinates(match.group(2) if match.group(2) else "")
                text = match.group(3) if match.group(3) else ""
                
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
        
        if widget.widget_type == "Button":
            # button rectangle
            self.canvas.create_rectangle(x1, y1, x2, y2, fill=widgets[widget.widget_type])
            # button text
            self.canvas.create_text(
                (x1 + x2) // 2,
                (y1 + y2) // 2,
                text=widget.text
            )
        elif widget.widget_type == "NewButton":
            # button rectangle
            self.canvas.create_rectangle(x1, y1, x2, y2, fill=widgets[widget.widget_type])
            # button text
            self.canvas.create_text(
                (x1 + x2) // 2,
                (y1 + y2) // 2,
                text=widget.text
            )

        # widget obj name in hpp code
        self.canvas.create_text(
            x1,
            y1 - 10,
            text=f"{widget.name} ({widget.widget_type})",
            anchor='sw'
        )

def draw_top_bar(self):
    self.canvas.create_rectangle(0, 0, screen_width * scale, topbar_offset * scale, fill='lightblue')
    self.canvas.create_text(screen_width * scale // 2, topbar_offset * scale // 2, text='Top Bar', fill='black')

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