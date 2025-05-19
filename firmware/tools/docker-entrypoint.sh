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

set -e # exit immediatelly on any failure

build_make() {
   cd ..
   mkdir -p build
   cd build
	cmake ..
   make "$@"
   exit 0 # Report success :)
}

build_ninja() {
   cd ..
   mkdir -p build
   cd build
	cmake -G Ninja ..
   ninja "$@"
   exit 0 # Report success :)
}

if [ "$1" = 'make' ]; then # build the default (or specified) target with make
   shift # remove the first item from $@ as we consumed it, we can then pass the rest on to make
   build_make "$@"
elif [[ $1 == -j* ]]; then # allow passing -j without typing make_default
   # dont shift here as we wanna pass the -j
   build_make "$@"
elif [ "$1" = 'ninja' ]; then # build the default (or specified) target with ninja
   shift # remove the first item from $@ as we consumed it, we can then pass the rest on to make
   build_ninja "$@"
fi

exec "$@"
