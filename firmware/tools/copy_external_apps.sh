#!/bin/bash

#
# Copyright (C) 2023 Bernd Herzog
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

try=1
while [ $try -le 60 ]
do
    if ls /dev/disk/by-id/*Portapack*part1 1> /dev/null 2>&1; then
        disk=$(ls /dev/disk/by-id/*Portapack*part1)
        part=$(readlink -f $disk)
        mountpoint=$(findmnt -f -o TARGET --noheadings $part)
        if [[ ! -z "$mountpoint" ]]; then

            echo "Copying external applications to" $mountpoint
            cp application/*.ppma $mountpoint

            echo "Unmounting" $mountpoint
            umount $mountpoint

            exit
        fi
    fi

    if [ "$(( $try %5 ))" -eq "1" ]; then
        echo "Please put the portapack into usb mode..."
    fi

    sleep 1

    try=$(( $try + 1 ))
done
