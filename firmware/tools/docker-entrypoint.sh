#!/bin/bash

#
# Copyright (C) 2024 u-foka
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

set -e # exit immediately on any failure

# 1. PARSE ARGUMENTS
# We separate arguments into two lists: 
# - CMAKE_OPTS: Anything starting with -D (e.g., -DFLASH_MB_SIZE=4)
# - BUILD_ARGS: Everything else (e.g., make, ninja, -j20)

CMAKE_OPTS=()
BUILD_ARGS=()

for arg in "$@"; do
    if [[ "$arg" == -D* ]]; then
        CMAKE_OPTS+=("$arg")
    else
        BUILD_ARGS+=("$arg")
    fi
done

# Reset the script arguments ($1, $2...) to contain only the BUILD_ARGS. This allows the original logic below to work exactly as before.
set -- "${BUILD_ARGS[@]}"

build_make() {
   cd ..
   mkdir -p build
   cd build
   cmake "${CMAKE_OPTS[@]}" ..
   make "$@"
   exit 0 
}

build_ninja() {
   cd ..
   mkdir -p build
   cd build
   cmake -G Ninja "${CMAKE_OPTS[@]}" ..
   ninja "$@"
   exit 0 
}


if [ "$1" = 'make' ]; then 
   # User explicitly typed 'make'
   shift 
   build_make "$@"

elif [[ $1 == -j* ]]; then 
   # User passed -j20 directly (Implied make)
   build_make "$@"

elif [ "$1" = 'ninja' ]; then 
   # User explicitly typed 'ninja'
   shift 
   build_ninja "$@"

elif [ ${#CMAKE_OPTS[@]} -gt 0 ] && [ ${#BUILD_ARGS[@]} -eq 0 ]; then
   # User passed ONLY CMake flags (e.g. "-DFLASH_MB_SIZE=4")
   # We assume they want to run a default 'make' build.
   build_make

fi

# Fallback for other commands (e.g. /bin/bash)
exec "$@"