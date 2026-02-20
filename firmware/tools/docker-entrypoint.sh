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

BUILD_DIR="build"
CMAKE_ARGS=()
BUILD_ARGS=()

parse_build_args() {
   # 1. PARSE BUILD ARGUMENTS
   # We separate arguments into two lists, and possibly get an override for the build dir:
   # - BUILD_DIR: Defaults to build, can be overriden (e.g., -B build-4mb, --workdir build-praline)
   # - CMAKE_ARGS: Anything starting with -D (e.g., -DFLASH_MB_SIZE=4)
   # - BUILD_ARGS: Everything else (e.g., make, ninja, -j20)

   while [[ $# -gt 0 ]]; do
      if [[ "$1" == -B ]] || [[ "$1" == "--workdir" ]]; then
         shift
         BUILD_DIR=$1
         shift
      elif [[ "$1" == -D* ]]; then
         CMAKE_ARGS+=("$1")
         shift
      else
         BUILD_ARGS+=("$1")
         shift
      fi
   done
}

build_make() {
   parse_build_args "$@"
   echo "Building in workdir: \"$BUILD_DIR\" using CMake options: \"${CMAKE_ARGS[@]}\" and make options: \"${BUILD_ARGS[@]}\""
   cd ..
   mkdir -p "$BUILD_DIR"
   cd "$BUILD_DIR"
   cmake "${CMAKE_ARGS[@]}" ..
   make "${BUILD_ARGS[@]}"
   exit 0 
}

build_ninja() {
   parse_build_args "$@"
   echo "Building in workdir: \"$BUILD_DIR\" using CMake options: \"${CMAKE_ARGS[@]}\" and ninja options: \"${BUILD_ARGS[@]}\""
   cd ..
   mkdir -p "$BUILD_DIR"
   cd "$BUILD_DIR"
   cmake -G Ninja "${CMAKE_ARGS[@]}" ..
   ninja "${BUILD_ARGS[@]}"
   exit 0 
}

if [ "$1" = 'make' ]; then 
   # User explicitly typed 'make'
   shift 
   build_make "$@"

elif [[ $1 == -* ]]; then 
   # User passed a switch not a command as first argument. Assuming make. (e.g., -j20)
   build_make "$@"

elif [ "$1" = 'ninja' ]; then 
   # User explicitly typed 'ninja'
   shift 
   build_ninja "$@"
fi

# Fallback for other commands (e.g. /bin/bash)
exec "$@"