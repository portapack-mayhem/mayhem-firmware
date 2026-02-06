#!/bin/sh

find firmware/common \
     firmware/baseband \
     firmware/application \
     firmware/test/application \
     firmware/test/baseband \
     \( -iname '*.h' -o -iname '*.hpp' -o -iname '*.c' -o -iname '*.cpp' \) \
     -print0 | \
     xargs -0 clang-format-18 -style=file -i

