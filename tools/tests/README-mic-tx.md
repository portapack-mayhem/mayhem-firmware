Mic TX host regression tests
============================

Requirements: Python 3, NumPy, SciPy, g++ with C++17/C++20 and UBSan support.
Run from the repository root:

```sh
PYTHONPATH=tools:tools/tests python3 -B -m unittest discover -s tools/tests -p 'test_mic_*.py' -v
```

The harnesses compile actual DSP loops and configuration/UI methods with host
hardware stubs. Public upstream commit 510d579d7405ca899963691513c46de74943e9c0
provides the prior implementation for regression comparisons; no private
experiment commits or build artifacts are required. The x8 test module supplies
the shared SSB harness; the x16 module tests the final reconstruction chain.

Checks cover independent FIR convolution and response, quantized spectra,
saturation/nonfinite inputs, USB/LSB mapping, unchanged FM/Hilbert/DMA code,
AM/DSB microphone and Roger-beep behavior, repeated allocation/replacement,
mutex initialization and controlled worker/configuration/beep overlap, saved
SSB bandwidth normalization, FM transitions and idle/TX message values.

Host scheduling does not reproduce target priority inheritance or cycle timing.
Numerical spectral checks are synthetic estimates, not RF measurements.
