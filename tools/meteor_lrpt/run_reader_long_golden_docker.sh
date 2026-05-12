#!/usr/bin/env bash
# CI / dev: compile and run Meteor LRPT host golden helpers inside Ubuntu (g++).
# Run from the repo root on a host where `docker` is on PATH (this script invokes `docker run`).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
docker run --rm -v "${ROOT}:/work" -w /work ubuntu:noble bash -lc '
  apt-get update -qq && apt-get install -y -qq gcc g++ python3 >/dev/null
  g++ -std=c++17 -O2 -I firmware/baseband -I firmware/common \
    tools/meteor_lrpt/test_reader_long_golden_host.cpp \
    firmware/baseband/meteor_lrpt/external_ring_ram.cpp \
    firmware/baseband/meteor_lrpt/meteor_deinterleaver.cpp \
    firmware/baseband/meteor_lrpt/meteor_lrpt_deinterleave_dispatch.cpp \
    firmware/baseband/meteor_lrpt/meteor_deinterleaver_reader.cpp \
    firmware/baseband/meteor_lrpt/meteor_soft_correlate.cpp \
    -o /tmp/reader_long_golden
  /tmp/reader_long_golden
  g++ -std=c++17 -O2 -I firmware/baseband \
    tools/meteor_lrpt/test_m2x_pipeline_host.cpp \
    firmware/baseband/meteor_lrpt/meteor_m2x_interleaved_pipeline.cpp \
    firmware/baseband/meteor_lrpt/meteor_viterbi12.cpp \
    firmware/baseband/meteor_lrpt/meteor_cc_decoder.cpp \
    firmware/baseband/meteor_lrpt/meteor_cc_encoder.cpp \
    firmware/baseband/meteor_lrpt/meteor_soft_correlate.cpp \
    firmware/baseband/meteor_lrpt/meteor_nrzm.cpp \
    firmware/baseband/meteor_lrpt/meteor_bpsk_ccsds_deframer.cpp \
    -o /tmp/m2x_pipeline_smoke
  /tmp/m2x_pipeline_smoke
  g++ -std=c++17 -O2 -I firmware/baseband -I firmware/common \
    tools/meteor_lrpt/test_m2x_interleaved_ram_pipeline_host.cpp \
    firmware/baseband/meteor_lrpt/external_ring_ram.cpp \
    firmware/baseband/meteor_lrpt/meteor_deinterleaver.cpp \
    firmware/baseband/meteor_lrpt/meteor_lrpt_deinterleave_dispatch.cpp \
    firmware/baseband/meteor_lrpt/meteor_deinterleaver_reader.cpp \
    firmware/baseband/meteor_lrpt/meteor_soft_correlate.cpp \
    firmware/baseband/meteor_lrpt/meteor_m2x_interleaved_pipeline.cpp \
    firmware/baseband/meteor_lrpt/meteor_viterbi12.cpp \
    firmware/baseband/meteor_lrpt/meteor_cc_decoder.cpp \
    firmware/baseband/meteor_lrpt/meteor_cc_encoder.cpp \
    firmware/baseband/meteor_lrpt/meteor_nrzm.cpp \
    firmware/baseband/meteor_lrpt/meteor_bpsk_ccsds_deframer.cpp \
    firmware/baseband/meteor_lrpt/ccsds_derandomize.cpp \
    firmware/baseband/meteor_lrpt/rs223_decode.cpp \
    -o /tmp/m2x_interleaved_ram_pipeline
  /tmp/m2x_interleaved_ram_pipeline -
  g++ -std=c++17 -O2 -I firmware/application -I firmware/application/meteor_lrpt_g4 \
    tools/meteor_lrpt/test_msumr_g4_host.cpp \
    firmware/application/meteor_lrpt_msumr.cpp \
    firmware/application/meteor_lrpt_g4/msumr_demux.cpp \
    -o /tmp/msumr_g4_host
  /tmp/msumr_g4_host
  /tmp/msumr_g4_host tools/meteor_lrpt/data/g4_standalone_apid68.cadu
  gcc -c -O2 -std=c99 -Ifirmware/application/meteor_lrpt_g4/third_party/tjpgd \
    firmware/application/meteor_lrpt_g4/third_party/tjpgd/tjpgd.c -o /tmp/tjpgd.o
  g++ -std=c++17 -O2 -Ifirmware/application/meteor_lrpt_g4/third_party/tjpgd \
    tools/meteor_lrpt/test_tjpgd_g4_host.cpp /tmp/tjpgd.o -o /tmp/test_tjpgd_g4_host
  /tmp/test_tjpgd_g4_host
  python3 tools/meteor_lrpt/compare_cadu.py --validate-soft-file /dev/null --soft-block-bytes 16384
'
