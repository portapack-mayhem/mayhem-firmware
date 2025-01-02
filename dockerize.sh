#!/bin/bash

#################################################
# This script aids building mayhem inside docker
#
# Basic usage:
# - Build dev container: ./dockerize.sh build
# - Build mayhem: ./dockerize.sh
#
# The image will be automatically build if it
# does not exist, but if the dockerfile changes
# it need to be rebuilt manually.
# 
# Advanced parameters:
# - Get a shell inside the build image to
#   inspect problems: ./dockerize.sh shell
# - Give additional parameters to the container:
#   ./dockerize.sh -j10
#   ./dockerize.sh ninja -j10
# - Use a different dockerfile:
#   ./dockerize.sh build dockerfile-other
# - Use a different cpu architecture:
#   ./dockerize.sh build dockerfile-nogit-arm arm64
#
# Environment variables:
# - It is possible to override the default image
#   name that is being used to build and run the
#   build container using an environment variable.
#   The default is 'portapack-dev'
#   Override by setting the following environment
#   variable: MAYHEM_DEV_DOCKER_IMAGE
#

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

DIR="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

IMAGE="${MAYHEM_DEV_DOCKER_IMAGE:-portapack-dev}"

build_image() {
   DOCKERFILE=${1:-dockerfile-nogit}
   PLATFORM=${2:-amd64}
   docker build --platform "linux/${PLATFORM}" -t "${IMAGE}" -f "${DOCKERFILE}" .
}

start_docker() {
   if [ -z "$(docker images -q ${IMAGE} 2> /dev/null)" ]; then
      build_image
   fi

   exec docker run -v "${DIR}:/havoc" -u "$(id -u):$(id -g)" -ti --rm "${IMAGE}" "$@"
}

if [ "$1" = 'shell' ]; then # open a shell into the container
   start_docker "bash -li"
elif [ "$1" = 'build' ]; then # build the default (or specified) target with ninja
   shift # remove the first item from $@ as we consumed it, we can then pass the rest on to make
   build_image "$@"
   exit $?
fi

start_docker "$@"
