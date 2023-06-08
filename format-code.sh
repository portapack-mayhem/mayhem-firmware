#!/bin/sh
find firmware/common firmware/baseband firmware/application firmware/test/application firmware/test/baseband -iname '*.h' -o -iname '*.hpp' -o -iname '*.cpp' -o -iname '*.c' | xargs clang-format-13 -style=file -i
