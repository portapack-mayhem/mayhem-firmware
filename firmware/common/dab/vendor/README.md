# Vendored third-party code

Nothing in this directory is formatted by the project's clang-format: the
`.clang-format` here sets `DisableFormat: true` so the files stay byte-identical
to upstream, both for licence reasons and so they can be re-fetched and diffed.

## kjmp2 - classic DAB audio

`kjmp2.c` / `kjmp2.h` - minimal MPEG-1/2 Audio Layer II decoder by
Martin J. Fiedler, version 1.1, copyright 2006-2013.

- Upstream: https://keyj.emphy.de/kjmp2/
- **Licence: zlib-style permissive.** Use for any purpose, origin must not be
  misrepresented, altered versions must be marked, notice must not be removed.
  Compatible with this repository's GPL-3.
- Vendored **unmodified**, so the "altered versions" clause does not apply.
- Integer only, ~6.4 KB code and ~12.3 KB of tables. DAB carries classic
  services as MPEG Layer II, one 1152-sample frame per 24 ms logical frame.
