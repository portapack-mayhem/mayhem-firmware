#!/usr/bin/env bash
# Summarize .text bytes per .obj from application.map (top contributors).
set -euo pipefail
MAP="${1:-build/firmware/application/application.map}"
if [[ ! -f "$MAP" ]]; then
	echo "missing $MAP" >&2
	exit 1
fi
awk '
/\.text/ && /\.obj$/ {
	# " .text    0xaddr 0xsize file.obj"
	sz = strtonum("0x" $3)
	if (sz > 0) {
		f = $NF
		sub(/.*\//, "", f)
		a[f] += sz
	}
}
END {
	for (f in a) print a[f], f
}' "$MAP" | sort -n | tail -40
