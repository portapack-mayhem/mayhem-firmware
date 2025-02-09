#!/bin/bash
for fname in $(ls ../graphics/); do
    fname=$(echo "$fname" | awk -F'.' '{print $1}')
    out=$(grep -r --exclude="bitmap.hpp" "$fname" ../* )
    ret=$?
    if [[ "$ret" -ne 0 ]]; then
        echo "$fname not found"
    fi
done

