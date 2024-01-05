#!/bin/bash

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
