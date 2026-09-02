# Host-side tests for the Meshtastic app. Nothing here needs a PortaPack: they run
# the firmware's own sources on an ordinary computer.
#
#   make            build and run everything
#   make test_mesh_regions && ./test_mesh_regions    one of them
#   make clean
#
# The sources come straight from firmware/, not from copies, so a test that passes
# here is testing the code that ships.

CXX      ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
# The router pulls in firmware headers that sit outside its own directory.
INC       = -I../../firmware/common -I../../firmware/application \
            -I../../firmware/application/apps/mesh

APP  = ../../firmware/application/apps/mesh
COMM = ../../firmware/common

# Self-contained: the test includes everything it needs as headers.
SIMPLE = test_lora_framing test_lora_ref test_lora_rssi test_synth_free

# These pull in real firmware translation units.
REGIONS_SRC = $(APP)/mesh_crypto.cpp
CRYPTO_SRC  = $(APP)/mesh_crypto.cpp
ROUTER_SRC  = $(APP)/mesh_router.cpp $(APP)/mesh_protocol.cpp \
              $(APP)/mesh_crypto.cpp $(APP)/mesh_nodedb.cpp $(APP)/mesh_pki.cpp

TESTS = test_lora_framing test_lora_ref test_lora_rssi \
        test_mesh_regions test_mesh_crypto test_mesh_router \
        test_mesh_telemetry test_mesh_partial

all: run

build: $(TESTS)

test_lora_framing: test_lora_framing.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_lora_ref: test_lora_ref.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_lora_rssi: test_lora_rssi.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_mesh_regions: test_mesh_regions.cpp $(REGIONS_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

test_mesh_crypto: test_mesh_crypto.cpp $(CRYPTO_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

test_mesh_router: test_mesh_router.cpp $(ROUTER_SRC)
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $^

test_mesh_telemetry: test_mesh_telemetry.cpp $(ROUTER_SRC)
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $^

test_mesh_partial: test_mesh_partial.cpp $(ROUTER_SRC)
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $^

# Run them all and stop at the first failure, so a red build says which one.
run: build
	@fail=0; for t in $(TESTS); do \
	  printf '=== %s\n' "$$t"; \
	  ./$$t || { echo "FAILED: $$t"; fail=1; break; }; \
	done; \
	[ $$fail -eq 0 ] && echo "=== all host tests passed"; exit $$fail

# The glyph table generator checks its own coverage; it needs Pillow and matplotlib.
font:
	python3 gen_font.py --list

clean:
	rm -f $(TESTS)

.PHONY: all build run font clean
