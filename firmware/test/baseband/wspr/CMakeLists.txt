# Standalone host tests: cmake -S firmware/test/baseband/wspr -B build_wspr_asan
cmake_minimum_required(VERSION 3.16)
project(wspr_host_tests LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(WSPR_SANITIZE "Enable address and undefined-behavior sanitizers" ON)
if(WSPR_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()
enable_testing()
get_filename_component(FIRMWARE "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(WSPR "${FIRMWARE}/baseband/wspr_lib")
add_library(wspr STATIC
    ${WSPR}/wspr_constants.c ${WSPR}/wspr_encode.c
    ${WSPR}/wspr_unpack.c ${WSPR}/wspr_decode.c)
# Host-only diagnostic prints inside wspr_decode (the M4 build never defines this).
target_compile_definitions(wspr PUBLIC WSPR_HOST_DIAG)
target_include_directories(wspr PUBLIC ${WSPR} ${FIRMWARE}/test/include)
add_executable(wspr_decode_test ../main.cpp ../wspr_decode_test.cpp ../wspr_lib_test.cpp)
target_link_libraries(wspr_decode_test PRIVATE wspr)
add_test(NAME wspr_decode COMMAND wspr_decode_test)

# Diagnostic screen-state machine (WSPR_RX_DIAG), header-only.
add_executable(wspr_diag_test ../main.cpp ../wspr_diag_test.cpp)
target_include_directories(wspr_diag_test PRIVATE
    ${FIRMWARE}/application/external/wspr_rx ${FIRMWARE}/test/include)
add_test(NAME wspr_diag COMMAND wspr_diag_test)

foreach(osr 1 2)
    add_executable(wspr_portapack_osr${osr}_test
        ../wspr_portapack_test.cpp ${FIRMWARE}/baseband/wspr_portapack.c)
    target_compile_definitions(wspr_portapack_osr${osr}_test PRIVATE WSPR_RX_TIME_OSR=${osr})
    target_link_libraries(wspr_portapack_osr${osr}_test PRIVATE wspr)
    add_test(NAME wspr_portapack_osr${osr} COMMAND wspr_portapack_osr${osr}_test)
endforeach()

# End-to-end ring streaming tests: sample-by-sample feed -> incremental scan ->
# ring snapshot -> snapshot decode. OSR2 is the shipped configuration.
add_executable(wspr_ring_stream_test
    ../wspr_ring_stream_test.cpp ${FIRMWARE}/baseband/wspr_portapack.c)
target_compile_definitions(wspr_ring_stream_test PRIVATE WSPR_RX_TIME_OSR=2)
target_link_libraries(wspr_ring_stream_test PRIVATE wspr)
# Shard the (long, ~18 min under ASAN) ring streaming suite so ctest -j runs
# the pieces in parallel. One executable, doctest name filters per shard;
# union of the shards must cover every TEST_CASE.
# IMPORTANT: each -tc filter must be passed as a SINGLE argument with no
# embedded literal quotes. A comma-separated pattern list inside one argument is
# what doctest expects; the previous `-tc="p1","p2"` form embedded literal
# quotation marks in the argument and matched ZERO test cases while CTest still
# reported PASS (false green). Found by the shard-verification harness.
add_test(NAME wspr_ring_stream_core COMMAND wspr_ring_stream_test
    "-tc=streaming: two slots*,streaming: decode at arbitrary*,streaming: single slot*,streaming: message change*,streaming: decodes a transmission with nonzero drift*,streaming: uint32 counter wrap*")
add_test(NAME wspr_ring_stream_stations COMMAND wspr_ring_stream_test
    "-tc=streaming: two same-start stations*,streaming: 60 minutes of noise-only*")
add_test(NAME wspr_ring_stream_fault COMMAND wspr_ring_stream_test
    "-tc=fault injection*,scan session*,bounded scan progress*")
add_test(NAME wspr_ring_stream_scan COMMAND wspr_ring_stream_test
    "-tc=scan cost*,scan staleness*,ring snapshot guard*,snapshot belt gate*,DIAG*")

# DEDUP TOLERANCE STUDY (S3 diagnostic, measurement only - no behaviour
# expectations on existing logic): records every scan-released candidate of
# the two-slot streaming fixtures to CSV (env WSPR_DEDUP_CSV).
add_executable(wspr_dedup_tol_test
    ../wspr_dedup_tol_test.cpp ${FIRMWARE}/baseband/wspr_portapack.c)
target_compile_definitions(wspr_dedup_tol_test PRIVATE WSPR_RX_TIME_OSR=2)
target_link_libraries(wspr_dedup_tol_test PRIVATE wspr)
add_test(NAME wspr_dedup_tol COMMAND wspr_dedup_tol_test)

# WSPR RX UI v1 fixed station table model (ui_station_table.hpp), header-only,
# zero allocation: capacity/eviction, unique-REPORT dedup, confirmation
# contract, MIN visibility filter.
add_executable(wspr_station_table_test ../main.cpp ../wspr_station_table_test.cpp)
target_include_directories(wspr_station_table_test PRIVATE
    ${FIRMWARE}/application/external/wspr_rx ${FIRMWARE}/test/include)
add_test(NAME wspr_station_table COMMAND wspr_station_table_test)

# WSPR RX UI v1 StationListView render harness (review fixes 3/4 + extras):
# the REAL ui_station_list.hpp + ui_station_table.hpp against a minimal
# ui::Widget/ui::Painter host shim (ui_shim/) with the REAL fixed_8x16
# glyphs. Covers wiring->report->row N=1, scroll at constant total with a
# scrollbar strip that survives rows/focus/blur, TTL display transition and
# eviction-above-selection viewport adjustment.
set(WSPR_UI_SHIM ${CMAKE_CURRENT_SOURCE_DIR}/ui_shim)
add_executable(wspr_station_list_view_test ../main.cpp ../wspr_station_list_view_test.cpp)
target_include_directories(wspr_station_list_view_test PRIVATE
    ${WSPR_UI_SHIM}
    ${FIRMWARE}/common
    ${FIRMWARE}/application/ui
    ${FIRMWARE}/application/external/wspr_rx
    ${FIRMWARE}/test/include)
target_sources(wspr_station_list_view_test PRIVATE
    ${WSPR_UI_SHIM}/ui_shim.cpp
    ${FIRMWARE}/common/ui_text.cpp
    ${FIRMWARE}/application/ui/ui_font_fixed_8x16.cpp)
add_test(NAME wspr_station_list_view COMMAND wspr_station_list_view_test)

# Native-font preview: renders the real StationListView with the real
# fixed_8x16 glyph data into a 240x320 PPM (convert to PNG with
# ../ppm_to_png.py). Not a ctest: run manually, e.g.
#   wspr_station_list_preview <out>.ppm
add_executable(wspr_station_list_preview ../wspr_station_list_preview.cpp)
target_include_directories(wspr_station_list_preview PRIVATE
    ${WSPR_UI_SHIM}
    ${FIRMWARE}/common
    ${FIRMWARE}/application/ui
    ${FIRMWARE}/application/external/wspr_rx
    ${FIRMWARE}/test/include)
target_sources(wspr_station_list_preview PRIVATE
    ${WSPR_UI_SHIM}/ui_shim.cpp
    ${FIRMWARE}/common/ui_text.cpp
    ${FIRMWARE}/application/ui/ui_font_fixed_8x16.cpp)
