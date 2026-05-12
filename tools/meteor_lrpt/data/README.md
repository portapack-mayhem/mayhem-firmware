# Meteor LRPT binary fixtures (optional)

Place **pinned** golden inputs here for CI or regression notes (not required for default builds):

- `*.c16` — IQ captures (Mayhem Capture).
- `*.c8` / `*.C8` — soft streams; block size **16384** (legacy) or **8192** (M2-x interleaved); validate with  
  `python tools/meteor_lrpt/compare_cadu.py --validate-soft-file tools/meteor_lrpt/data/your.C8 --soft-block-bytes 8192`
- `*.cadu` — reference CADU from SatDump or from `test_m2x_interleaved_ram_pipeline_host.cpp` output.
- `g4_tiny_baseline.jpg` — **8×8** RGB baseline JPEG (regenerate: `tools/meteor_lrpt/docker_gen_tjpgd_golden.sh` in Docker) for `test_msumr_g4_host.cpp` and **`test_tjpgd_g4_host`** golden; not a full pass product.
- `g4_standalone_apid68.cadu` — single **1020 B** Mayhem CADU record (ASM + standalone CCSDS packet, APID 68, minimal JPEG SOI/EOI); run `test_msumr_g4_host` with this path as argv[1].

Document SHA-256 in [`../README.md`](../README.md) regression table when adding files.
