# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/opt/portapack-mayhem/hackrf/firmware/libopencm3"
  "/opt/portapack-mayhem/hackrf-portapack/libopencm3_hackrf_usb-prefix/src/libopencm3_hackrf_usb-build"
  "/opt/portapack-mayhem/hackrf-portapack/libopencm3_hackrf_usb-prefix"
  "/opt/portapack-mayhem/hackrf-portapack/libopencm3_hackrf_usb-prefix/tmp"
  "/opt/portapack-mayhem/hackrf-portapack/libopencm3_hackrf_usb-prefix/src/libopencm3_hackrf_usb-stamp"
  "/opt/portapack-mayhem/hackrf-portapack/libopencm3_hackrf_usb-prefix/src"
  "/opt/portapack-mayhem/hackrf-portapack/libopencm3_hackrf_usb-prefix/src/libopencm3_hackrf_usb-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/opt/portapack-mayhem/hackrf-portapack/libopencm3_hackrf_usb-prefix/src/libopencm3_hackrf_usb-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/opt/portapack-mayhem/hackrf-portapack/libopencm3_hackrf_usb-prefix/src/libopencm3_hackrf_usb-stamp${cfgdir}") # cfgdir has leading slash
endif()
