#!/usr/bin/env bash
# Compare SHA256SUMS.txt from two share-out/artifacts runs (same filenames). Usage:
#   ./scripts/stability-compare-builds.sh share-out/artifacts/SHA256SUMS.txt share-out/artifacts/SHA256SUMS.prev.txt
set -euo pipefail
if [[ $# -ne 2 ]]; then
	echo "usage: $0 <SHA256SUMS_A> <SHA256SUMS_B>" >&2
	exit 2
fi
diff -u "$1" "$2" && echo "stability-compare-builds: checksum files match." || {
	echo "stability-compare-builds: DIFFER (intentional version bump is OK)." >&2
	exit 1
}
