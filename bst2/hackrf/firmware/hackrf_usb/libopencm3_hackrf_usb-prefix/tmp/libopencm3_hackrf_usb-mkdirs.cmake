# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/havoc/hackrf/firmware/libopencm3"
  "/havoc/bst2/hackrf/firmware/hackrf_usb/libopencm3_hackrf_usb-prefix/src/libopencm3_hackrf_usb-build"
  "/havoc/bst2/hackrf/firmware/hackrf_usb/libopencm3_hackrf_usb-prefix"
  "/havoc/bst2/hackrf/firmware/hackrf_usb/libopencm3_hackrf_usb-prefix/tmp"
  "/havoc/bst2/hackrf/firmware/hackrf_usb/libopencm3_hackrf_usb-prefix/src/libopencm3_hackrf_usb-stamp"
  "/havoc/bst2/hackrf/firmware/hackrf_usb/libopencm3_hackrf_usb-prefix/src"
  "/havoc/bst2/hackrf/firmware/hackrf_usb/libopencm3_hackrf_usb-prefix/src/libopencm3_hackrf_usb-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/havoc/bst2/hackrf/firmware/hackrf_usb/libopencm3_hackrf_usb-prefix/src/libopencm3_hackrf_usb-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/havoc/bst2/hackrf/firmware/hackrf_usb/libopencm3_hackrf_usb-prefix/src/libopencm3_hackrf_usb-stamp${cfgdir}") # cfgdir has leading slash
endif()
