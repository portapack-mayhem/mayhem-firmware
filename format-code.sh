#!/bin/sh
find firmware/common firmware/baseband firmware/application firmware/test -iname '*.h' -o -iname '*.hpp' -o -iname '*.cpp' -o -iname '*.c' | xargs clang-format -style=file -i
