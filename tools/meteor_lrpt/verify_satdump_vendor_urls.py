#!/usr/bin/env python3
"""
Verify raw.githubusercontent.com URLs listed in SATDUMP_VENDOR.md still return HTTP 200.
Run from repo root: python tools/meteor_lrpt/verify_satdump_vendor_urls.py
"""
from __future__ import annotations

import re
import sys
import urllib.error
import urllib.request
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
VENDOR_MD = REPO_ROOT / "firmware" / "baseband" / "meteor_lrpt" / "SATDUMP_VENDOR.md"


class _HeadRequest(urllib.request.Request):
    """HEAD for Python < 3.8 (no Request(method=...))."""

    def get_method(self) -> str:  # noqa: N802 — urllib API
        return "HEAD"


def _head_request(url: str) -> urllib.request.Request:
    if sys.version_info >= (3, 8):
        return urllib.request.Request(url, method="HEAD")
    return _HeadRequest(url)


def main() -> int:
    if not VENDOR_MD.is_file():
        print(f"missing {VENDOR_MD}", file=sys.stderr)
        return 2
    text = VENDOR_MD.read_text(encoding="utf-8")
    urls = sorted(set(re.findall(r"https://raw\.githubusercontent\.com/SatDump/SatDump/[a-f0-9]{40}/[^\s)>`]+", text)))
    if not urls:
        print("no raw SatDump URLs found in SATDUMP_VENDOR.md", file=sys.stderr)
        return 2
    failures = 0
    for url in urls:
        req = _head_request(url)
        try:
            with urllib.request.urlopen(req, timeout=30) as r:
                code = r.getcode()
        except urllib.error.HTTPError as e:
            code = e.code
        except OSError as e:
            print(f"ERR {url}\n  {e}", file=sys.stderr)
            failures += 1
            continue
        if code != 200:
            print(f"FAIL {code} {url}", file=sys.stderr)
            failures += 1
        else:
            print(f"OK   {url}")
    if failures:
        print(f"\n{failures} URL(s) failed", file=sys.stderr)
        return 1
    print(f"\nAll {len(urls)} SatDump vendor URL(s) OK.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
