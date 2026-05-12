# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/havoc/hackrf/firmware/libopencm3"
  "/havoc/bst2/hackrf/firmware/blinky/libopencm3_blinky-prefix/src/libopencm3_blinky-build"
  "/havoc/bst2/hackrf/firmware/blinky/libopencm3_blinky-prefix"
  "/havoc/bst2/hackrf/firmware/blinky/libopencm3_blinky-prefix/tmp"
  "/havoc/bst2/hackrf/firmware/blinky/libopencm3_blinky-prefix/src/libopencm3_blinky-stamp"
  "/havoc/bst2/hackrf/firmware/blinky/libopencm3_blinky-prefix/src"
  "/havoc/bst2/hackrf/firmware/blinky/libopencm3_blinky-prefix/src/libopencm3_blinky-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/havoc/bst2/hackrf/firmware/blinky/libopencm3_blinky-prefix/src/libopencm3_blinky-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/havoc/bst2/hackrf/firmware/blinky/libopencm3_blinky-prefix/src/libopencm3_blinky-stamp${cfgdir}") # cfgdir has leading slash
endif()
