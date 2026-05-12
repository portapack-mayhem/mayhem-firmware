#!/usr/bin/env bash
# Regenerate g4_tiny_baseline.jpg (PIL 8x8 RGB) and print tjpgd decode constants for test_tjpgd_g4_host.cpp.
set -euo pipefail
cd "$(dirname "$0")/../.."
apt-get update -qq
apt-get install -y -qq python3-pil gcc g++ >/dev/null
python3 - <<'PY'
from PIL import Image

Image.new("RGB", (8, 8), (128, 128, 128)).save(
    "tools/meteor_lrpt/data/g4_tiny_baseline.jpg", "JPEG", quality=85
)
PY
gcc -c -O2 -std=c99 -Wall -Wextra \
  -Ifirmware/application/meteor_lrpt_g4/third_party/tjpgd \
  firmware/application/meteor_lrpt_g4/third_party/tjpgd/tjpgd.c -o /tmp/tjpgd.o
g++ -std=c++17 -O2 -Wall -Wextra \
  -Ifirmware/application/meteor_lrpt_g4/third_party/tjpgd \
  tools/meteor_lrpt/test_tjpgd_g4_host.cpp /tmp/tjpgd.o -o /tmp/test_tjpgd_g4_host
/tmp/test_tjpgd_g4_host
