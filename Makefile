# DyMeN-Sim Makefile
# Builds NS-3 simulations using Homebrew-installed NS-3 3.46
#
# Requirements:
#   - NS-3 3.46 (install via Homebrew: brew install ns-3)
#   - Python 3.8+ with packages: pandas, numpy, matplotlib, seaborn, scipy
#
# Override NS-3 path if needed:
#   make NS3_PATH=/custom/path all

# Compiler (force x86_64 for Rosetta compatibility on Apple Silicon)
CXX = clang++
CXXFLAGS = -std=c++20 -Wall -O2 -arch x86_64

# NS-3 modules we'll use
NS3_MODULES = core network internet wifi mobility aodv olsr dsdv applications propagation flow-monitor mesh

# NS-3 installation paths (Homebrew default, override with NS3_PATH=...)
NS3_PATH ?= /usr/local/Cellar/ns-3/3.46
NS3_INCLUDE = -I$(NS3_PATH)/include -I/usr/local/include
NS3_LIBDIR = -L$(NS3_PATH)/lib
NS3_LIBS = -lns3.46-core -lns3.46-network -lns3.46-internet -lns3.46-wifi \
           -lns3.46-mobility -lns3.46-aodv -lns3.46-olsr -lns3.46-dsdv -lns3.46-applications \
           -lns3.46-propagation -lns3.46-flow-monitor -lns3.46-point-to-point -lns3.46-mesh

# Note: SGP4 library not included in reproducibility package
# Satellite mobility is pre-computed and embedded in simulation code

# Directories
SRC_DIR = src
BUILD_DIR = build
RESULTS_DIR = results

# Source files (exclude library files that don't have main())
ALL_SOURCES = $(wildcard $(SRC_DIR)/*.cc)
LIBRARY_FILES = $(SRC_DIR)/static-isl-routing.cc \
                $(SRC_DIR)/isl-topology-generator.cc \
                $(SRC_DIR)/isl-network-creator.cc \
                $(SRC_DIR)/static-routing-protocol.cc \
                $(SRC_DIR)/olsr-routing-protocol.cc \
                $(SRC_DIR)/aodv-routing-protocol.cc \
                $(SRC_DIR)/dsdv-routing-protocol.cc \
                $(SRC_DIR)/packet-tracer.cc
SOURCES = $(filter-out $(LIBRARY_FILES),$(ALL_SOURCES))
TARGETS = $(patsubst $(SRC_DIR)/%.cc,$(BUILD_DIR)/%,$(SOURCES))

# Default target
.PHONY: all
all: directories $(TARGETS)

# Create necessary directories
.PHONY: directories
directories:
	@mkdir -p $(BUILD_DIR) $(RESULTS_DIR)

# Compile rule (default for simple single-file programs)
$(BUILD_DIR)/%: $(SRC_DIR)/%.cc | directories
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Special targets for failure simulations (same compilation, explicitly listed for clarity)
$(BUILD_DIR)/mesh-baseline-failures: $(SRC_DIR)/mesh-baseline-failures.cc | directories
	@echo "Compiling $< (with Bernoulli error model)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

$(BUILD_DIR)/mesh-olsr-failures: $(SRC_DIR)/mesh-olsr-failures.cc | directories
	@echo "Compiling $< (with Bernoulli error model)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Special rules for satellite-related programs (require SGP4 library)
$(BUILD_DIR)/test-satellite-mobility-model: $(SRC_DIR)/test-satellite-mobility-model.cc \
                                             $(SRC_DIR)/satellite-mobility-model.cc \
                                             $(SGP4_SRC) | directories
	@echo "Compiling $< (with SGP4 orbital mechanics)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(SGP4_INCLUDE) $< \
	       $(SRC_DIR)/satellite-mobility-model.cc $(SGP4_SRC) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

$(BUILD_DIR)/test-walker-constellation: $(SRC_DIR)/test-walker-constellation.cc \
                                        $(SRC_DIR)/walker-constellation-helper.cc \
                                        $(SRC_DIR)/satellite-mobility-model.cc \
                                        $(SGP4_SRC) | directories
	@echo "Compiling $< (Walker-Delta constellation test)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(SGP4_INCLUDE) $< \
	       $(SRC_DIR)/walker-constellation-helper.cc \
	       $(SRC_DIR)/satellite-mobility-model.cc $(SGP4_SRC) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

$(BUILD_DIR)/satellite-mobility-model: $(SRC_DIR)/satellite-mobility-model.cc $(SGP4_SRC) | directories
	@echo "Compiling $< (satellite mobility model library)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(SGP4_INCLUDE) $< $(SGP4_SRC) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Ground station support (Day 3 - coordinate transformations)
$(BUILD_DIR)/test-ground-station: $(SRC_DIR)/test-ground-station.cc \
                                  $(SRC_DIR)/coordinate-transforms.cc | directories
	@echo "Compiling $< (ground station coordinate transforms test)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/coordinate-transforms.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 2 - Dynamic elevation tracking test
$(BUILD_DIR)/test-dynamic-elevation: $(SRC_DIR)/test-dynamic-elevation.cc \
                                     $(SRC_DIR)/coordinate-transforms.cc | directories
	@echo "Compiling $< (dynamic elevation tracking for moving nodes)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/coordinate-transforms.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 2 - Reusable patterns validation
$(BUILD_DIR)/test-phase2-patterns: $(SRC_DIR)/test-phase2-patterns.cc $(LEO_DEPS) | directories
	@echo "Compiling $< (Phase 2 reusable patterns test)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(SGP4_INCLUDE) $< $(LEO_DEPS) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 2 Days 3-5 - Elevation-dependent channel model test
$(BUILD_DIR)/test-channel-model: $(SRC_DIR)/test-channel-model.cc \
                                 $(SRC_DIR)/satellite-ground-propagation-loss-model.cc \
                                 $(SRC_DIR)/coordinate-transforms.cc | directories
	@echo "Compiling $< (elevation-dependent channel model test)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/satellite-ground-propagation-loss-model.cc \
	       $(SRC_DIR)/coordinate-transforms.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Full LEO network simulations (Day 4 - integration)
# Combines: satellites + ground stations + routing + traffic
LEO_DEPS = $(SRC_DIR)/walker-constellation-helper.cc \
           $(SRC_DIR)/satellite-mobility-model.cc \
           $(SRC_DIR)/coordinate-transforms.cc \
           $(SGP4_SRC)

# Phase 2 extended dependencies (includes channel model + TEME mobility)
PHASE2_DEPS = $(LEO_DEPS) \
              $(SRC_DIR)/satellite-ground-propagation-loss-model.cc \
              $(TEME_MOBILITY_SRCS)

$(BUILD_DIR)/test-leo-network: $(SRC_DIR)/test-leo-network.cc $(LEO_DEPS) | directories
	@echo "Compiling $< (LEO network integration tests)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(SGP4_INCLUDE) $< $(LEO_DEPS) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

$(BUILD_DIR)/leo-mesh-baseline: $(SRC_DIR)/leo-mesh-baseline.cc $(LEO_DEPS) | directories
	@echo "Compiling $< (LEO AODV simulation)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(SGP4_INCLUDE) $< $(LEO_DEPS) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

$(BUILD_DIR)/leo-mesh-olsr: $(SRC_DIR)/leo-mesh-olsr.cc $(LEO_DEPS) | directories
	@echo "Compiling $< (LEO OLSR simulation)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(SGP4_INCLUDE) $< $(LEO_DEPS) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 2 - Prototype with 20 mobile mesh nodes (now with realistic channel model)
$(BUILD_DIR)/phase2-prototype: $(SRC_DIR)/phase2-prototype.cc $(PHASE2_DEPS) | directories
	@echo "Compiling $< (Phase 2 prototype - decentralized access with realistic channel)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(SGP4_INCLUDE) $< $(PHASE2_DEPS) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 2 - Fast solution test (static ground stations with realistic channel)
$(BUILD_DIR)/test-channel-static-nodes: $(SRC_DIR)/test-channel-static-nodes.cc $(PHASE2_DEPS) | directories
	@echo "Compiling $< (channel model validation with static ground stations)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(SGP4_INCLUDE) $< $(PHASE2_DEPS) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# TEME-aware mobility model dependencies
TEME_MOBILITY_SRCS = $(SRC_DIR)/teme-aware-random-waypoint-mobility-model.cc

# TEME mobility model test
$(BUILD_DIR)/test-teme-mobility: $(SRC_DIR)/test-teme-mobility.cc \
                                 $(TEME_MOBILITY_SRCS) \
                                 $(SRC_DIR)/coordinate-transforms.cc | directories
	@echo "Compiling $< (TEME-aware mobility model test)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(TEME_MOBILITY_SRCS) \
	       $(SRC_DIR)/coordinate-transforms.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 3 - Static ISL Routing Test (TDD Day 3)
$(BUILD_DIR)/test-static-isl-routing: $(SRC_DIR)/test-static-isl-routing.cc \
                                      $(SRC_DIR)/static-isl-routing.cc \
                                      $(SRC_DIR)/isl-topology-generator.cc | directories
	@echo "Compiling $< (static ISL routing - TDD RED phase)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/static-isl-routing.cc \
	       $(SRC_DIR)/isl-topology-generator.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 3 - ISL Links Test (TDD Day 4)
$(BUILD_DIR)/test-isl-links: $(SRC_DIR)/test-isl-links.cc | directories
	@echo "Compiling $< (ISL link properties and throughput - TDD RED phase)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 3 - ISL Network Creation Test (TDD Day 4)
$(BUILD_DIR)/test-isl-network-creation: $(SRC_DIR)/test-isl-network-creation.cc \
                                        $(SRC_DIR)/isl-network-creator.cc \
                                        $(SRC_DIR)/isl-topology-generator.cc \
                                        $(SRC_DIR)/static-isl-routing.cc | directories
	@echo "Compiling $< (ISL network creation integration test)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/isl-network-creator.cc \
	       $(SRC_DIR)/isl-topology-generator.cc \
	       $(SRC_DIR)/static-isl-routing.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 3 - ISL Route Installation Test (TDD Day 5)
$(BUILD_DIR)/test-isl-route-installation: $(SRC_DIR)/test-isl-route-installation.cc \
                                          $(SRC_DIR)/isl-network-creator.cc \
                                          $(SRC_DIR)/isl-topology-generator.cc \
                                          $(SRC_DIR)/static-isl-routing.cc | directories
	@echo "Compiling $< (ISL route installation test - Day 5)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/isl-network-creator.cc \
	       $(SRC_DIR)/isl-topology-generator.cc \
	       $(SRC_DIR)/static-isl-routing.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Test target
.PHONY: test
test: $(BUILD_DIR)/test-hello
	@echo "\n━━━ Running NS-3 Installation Test ━━━"
	@$(BUILD_DIR)/test-hello

# Phase 3 TDD test target
.PHONY: test-phase3-day3
test-phase3-day3: $(BUILD_DIR)/test-static-isl-routing
	@echo "\n━━━ Running Phase 3 Day 3 TDD Tests (Static ISL Routing) ━━━"
	@$(BUILD_DIR)/test-static-isl-routing

.PHONY: test-phase3-day4
test-phase3-day4: $(BUILD_DIR)/test-isl-links $(BUILD_DIR)/test-isl-network-creation
	@echo "\n━━━ Running Phase 3 Day 4 TDD Tests (ISL Link Creation) ━━━"
	@$(BUILD_DIR)/test-isl-links
	@echo "\n━━━ Running Phase 3 Day 4 Integration Test (ISL Network Creation) ━━━"
	@$(BUILD_DIR)/test-isl-network-creation

.PHONY: test-phase3-day5-quick
test-phase3-day5-quick: $(BUILD_DIR)/test-isl-route-installation
	@echo "\n━━━ Running Phase 3 Day 5 Quick Tests (Route Installation) ━━━"
	@$(BUILD_DIR)/test-isl-route-installation

# Phase 3 - Routing table analysis
$(BUILD_DIR)/analyze-routing-tables: $(SRC_DIR)/analyze-routing-tables.cc \
                                     $(SRC_DIR)/static-isl-routing.cc \
                                     $(SRC_DIR)/isl-topology-generator.cc | directories
	@echo "Compiling $< (routing table analysis)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/static-isl-routing.cc \
	       $(SRC_DIR)/isl-topology-generator.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 3 - ISL Static Baseline (Day 5 - full end-to-end simulation)
ISL_STATIC_BASELINE_SRCS = $(SRC_DIR)/isl-static-baseline.cc \
                           $(SRC_DIR)/isl-network-creator.cc \
                           $(SRC_DIR)/static-isl-routing.cc \
                           $(SRC_DIR)/isl-topology-generator.cc

$(BUILD_DIR)/isl-static-baseline: $(ISL_STATIC_BASELINE_SRCS) | directories
	@echo "Compiling ISL Static Baseline (full simulation - Day 5)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(ISL_STATIC_BASELINE_SRCS) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 3 Week 18 - OLSR ISL Routing Test (TDD Day 1-2)
$(BUILD_DIR)/test-isl-olsr-routing: $(SRC_DIR)/test-isl-olsr-routing.cc \
                                    $(SRC_DIR)/isl-network-creator.cc \
                                    $(SRC_DIR)/isl-topology-generator.cc \
                                    $(SRC_DIR)/static-isl-routing.cc | directories
	@echo "Compiling $< (OLSR ISL routing - TDD RED phase)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/isl-network-creator.cc \
	       $(SRC_DIR)/isl-topology-generator.cc \
	       $(SRC_DIR)/static-isl-routing.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 3 Week 18 - ISL OLSR Baseline (full simulation)
ISL_OLSR_BASELINE_SRCS = $(SRC_DIR)/isl-olsr-baseline.cc \
                         $(SRC_DIR)/isl-network-creator.cc \
                         $(SRC_DIR)/isl-topology-generator.cc \
                         $(SRC_DIR)/static-isl-routing.cc

$(BUILD_DIR)/isl-olsr-baseline: $(ISL_OLSR_BASELINE_SRCS) | directories
	@echo "Compiling ISL OLSR Baseline (full simulation - Week 18)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(ISL_OLSR_BASELINE_SRCS) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Phase 3 Week 18 test target
.PHONY: test-week18-day1
test-week18-day1: $(BUILD_DIR)/test-isl-olsr-routing
	@echo "\n━━━ Running Week 18 Day 1-2 TDD Tests (OLSR ISL Routing) ━━━"
	@$(BUILD_DIR)/test-isl-olsr-routing

# Phase 3 Week 20 - Link Usage Analyzer Test (TDD Day 1)
$(BUILD_DIR)/test-link-usage-analyzer: $(SRC_DIR)/test-link-usage-analyzer.cc \
                                       $(SRC_DIR)/isl-network-creator.cc \
                                       $(SRC_DIR)/isl-topology-generator.cc \
                                       $(SRC_DIR)/static-isl-routing.cc | directories
	@echo "Compiling $< (Link usage analyzer test - TDD RED phase)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/isl-network-creator.cc \
	       $(SRC_DIR)/isl-topology-generator.cc \
	       $(SRC_DIR)/static-isl-routing.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

.PHONY: test-week20-day1
test-week20-day1: $(BUILD_DIR)/test-link-usage-analyzer
	@echo "\n━━━ Running Week 20 Day 1 TDD Tests (Link Usage Analyzer) ━━━"
	@$(BUILD_DIR)/test-link-usage-analyzer

# Phase 3 Week 20 - Topology Connectivity Test
$(BUILD_DIR)/test-topology-connectivity: $(SRC_DIR)/test-topology-connectivity.cc \
                                         $(SRC_DIR)/isl-topology-generator.cc \
                                         $(SRC_DIR)/static-isl-routing.cc | directories
	@echo "Compiling $< (Topology connectivity test - TDD RED phase)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/isl-topology-generator.cc \
	       $(SRC_DIR)/static-isl-routing.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

.PHONY: test-week20-topology
test-week20-topology: $(BUILD_DIR)/test-topology-connectivity
	@echo "\n━━━ Running Week 20 Topology Connectivity Tests ━━━"
	@$(BUILD_DIR)/test-topology-connectivity

# Phase 3 Week 20 - ISL Link Usage Analyzer (full simulation with tracing)
ISL_LINK_USAGE_ANALYZER_SRCS = $(SRC_DIR)/isl-link-usage-analyzer.cc \
                               $(SRC_DIR)/isl-network-creator.cc \
                               $(SRC_DIR)/isl-topology-generator.cc \
                               $(SRC_DIR)/static-isl-routing.cc

$(BUILD_DIR)/isl-link-usage-analyzer: $(ISL_LINK_USAGE_ANALYZER_SRCS) | directories
	@echo "Compiling ISL Link Usage Analyzer (with packet tracing)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(ISL_LINK_USAGE_ANALYZER_SRCS) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# ============================================================================
# Phase 4 Week 21 - Protocol Abstraction Layer (TDD)
# ============================================================================

# Protocol abstraction dependencies
PROTOCOL_FILES = $(SRC_DIR)/routing-protocol-factory.cc \
                 $(SRC_DIR)/static-routing-protocol.cc \
                 $(SRC_DIR)/olsr-routing-protocol.cc \
                 $(SRC_DIR)/aodv-routing-protocol.cc

# Week 21 Day 3 - OLSR Protocol Wrapper Test
$(BUILD_DIR)/test-olsr-routing-wrapper: test/test-olsr-routing-wrapper.cc \
                                        $(SRC_DIR)/olsr-routing-protocol.cc \
                                        $(SRC_DIR)/isl-network-creator.cc \
                                        $(SRC_DIR)/isl-topology-generator.cc \
                                        $(SRC_DIR)/static-isl-routing.cc | directories
	@echo "Compiling $< (OLSR protocol wrapper test - TDD Day 3)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/olsr-routing-protocol.cc \
	       $(SRC_DIR)/isl-network-creator.cc \
	       $(SRC_DIR)/isl-topology-generator.cc \
	       $(SRC_DIR)/static-isl-routing.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

.PHONY: test-week21-day3
test-week21-day3: $(BUILD_DIR)/test-olsr-routing-wrapper
	@echo "\n━━━ Running Week 21 Day 3 TDD Tests (OLSR Protocol Wrapper) ━━━"
	@$(BUILD_DIR)/test-olsr-routing-wrapper

# Week 21 Day 4 - AODV Protocol Wrapper Test
$(BUILD_DIR)/test-aodv-routing-wrapper: test/test-aodv-routing-wrapper.cc \
                                        $(SRC_DIR)/aodv-routing-protocol.cc | directories
	@echo "Compiling $< (AODV protocol wrapper test - TDD Day 4)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/aodv-routing-protocol.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

.PHONY: test-week21-day4
test-week21-day4: $(BUILD_DIR)/test-aodv-routing-wrapper
	@echo "\n━━━ Running Week 21 Day 4 TDD Tests (AODV Protocol Wrapper) ━━━"
	@$(BUILD_DIR)/test-aodv-routing-wrapper

# ============================================================================
# Phase 4 Week 22 - HWMP Protocol and Ground Layer Integration (TDD)
# ============================================================================

# Week 22 Day 1-2 - HWMP Protocol Wrapper Test
$(BUILD_DIR)/test-hwmp-routing-wrapper: test/test-hwmp-routing-wrapper.cc \
                                        $(SRC_DIR)/hwmp-routing-protocol.cc \
                                        $(SRC_DIR)/static-routing-protocol.cc \
                                        $(SRC_DIR)/olsr-routing-protocol.cc \
                                        $(SRC_DIR)/aodv-routing-protocol.cc | directories
	@echo "Compiling $< (HWMP protocol wrapper test - TDD Day 1-2)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/hwmp-routing-protocol.cc \
	       $(SRC_DIR)/static-routing-protocol.cc \
	       $(SRC_DIR)/olsr-routing-protocol.cc \
	       $(SRC_DIR)/aodv-routing-protocol.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

.PHONY: test-week22-day2
test-week22-day2: $(BUILD_DIR)/test-hwmp-routing-wrapper
	@echo "\n━━━ Running Week 22 Day 1-2 TDD Tests (HWMP Protocol Wrapper) ━━━"
	@$(BUILD_DIR)/test-hwmp-routing-wrapper

# ============================================================================
# Phase 4 Week 23 - DSDV Protocol (TDD)
# ============================================================================

# Week 23 - DSDV Protocol Wrapper Test
$(BUILD_DIR)/test-dsdv-routing-wrapper: test/test-dsdv-routing-wrapper.cc \
                                        $(SRC_DIR)/dsdv-routing-protocol.cc | directories
	@echo "Compiling $< (DSDV protocol wrapper test - TDD Week 23)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/dsdv-routing-protocol.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

.PHONY: test-week23
test-week23: $(BUILD_DIR)/test-dsdv-routing-wrapper
	@echo "\n━━━ Running Week 23 TDD Tests (DSDV Protocol Wrapper) ━━━"
	@$(BUILD_DIR)/test-dsdv-routing-wrapper

# ============================================================================
# Phase 6 Week 27 - PacketTracer for NRL Metrics (TDD)
# ============================================================================

# Week 27 - PacketTracer Unit Tests
$(BUILD_DIR)/test-packet-tracer: tests/test-packet-tracer.cc \
                                 $(SRC_DIR)/packet-tracer.cc | directories
	@echo "Compiling $< (PacketTracer unit tests - TDD Week 27)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $< \
	       $(SRC_DIR)/packet-tracer.cc \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

.PHONY: test-nc9
test-nc9: $(BUILD_DIR)/test-packet-tracer
	@echo "\n━━━ Running Week 27 TDD Tests (PacketTracer) ━━━"
	@$(BUILD_DIR)/test-packet-tracer

# Week 21-22 - Unified Simulation (factory-based protocol selection + ground layer)
# NC9/NC10 reproduction - includes only essential protocols (AODV, OLSR, DSDV)
UNIFIED_SIMULATION_SRCS = $(SRC_DIR)/unified-simulation.cc \
                          $(SRC_DIR)/static-routing-protocol.cc \
                          $(SRC_DIR)/olsr-routing-protocol.cc \
                          $(SRC_DIR)/aodv-routing-protocol.cc \
                          $(SRC_DIR)/dsdv-routing-protocol.cc \
                          $(SRC_DIR)/isl-network-creator.cc \
                          $(SRC_DIR)/isl-topology-generator.cc \
                          $(SRC_DIR)/static-isl-routing.cc \
                          $(SRC_DIR)/packet-tracer.cc

$(BUILD_DIR)/unified-simulation: $(UNIFIED_SIMULATION_SRCS) | directories
	@echo "Compiling unified-simulation (factory-based protocol selection + ground layer)..."
	$(CXX) $(CXXFLAGS) $(NS3_INCLUDE) $(UNIFIED_SIMULATION_SRCS) \
	       $(NS3_LIBDIR) $(NS3_LIBS) -o $@
	@echo "✓ Built: $@"

# Clean target
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "✓ Clean complete"

# Clean all (including results)
.PHONY: distclean
distclean: clean
	@echo "Cleaning results..."
	@rm -rf $(RESULTS_DIR)
	@echo "✓ Deep clean complete"

# Info target - show build configuration
.PHONY: info
info:
	@echo "DyMeN-Sim Build Configuration"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "NS-3 Version: $$(pkg-config --modversion ns3-core)"
	@echo "Compiler: $(CXX)"
	@echo "C++ Standard: C++17"
	@echo "NS-3 Modules: $(NS3_MODULES)"
	@echo ""
	@echo "Directories:"
	@echo "  Source: $(SRC_DIR)"
	@echo "  Build: $(BUILD_DIR)"
	@echo "  Results: $(RESULTS_DIR)"
	@echo ""
	@echo "Available targets:"
	@echo "  make all      - Build all simulations"
	@echo "  make test     - Run installation test"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make info     - Show this information"

# Help target
.PHONY: help
help: info

# Prevent make from deleting intermediate files
.PRECIOUS: $(BUILD_DIR)/%
