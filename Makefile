CXX := clang++

# Optimized build with symbols and reliable call stacks for perf.
CXXFLAGS := -std=c++17 \
            -O2 \
            -g \
            -fno-omit-frame-pointer \
            -DNDEBUG \
            -Wall \
            -Wextra \
            -Wpedantic \
            -Iinclude

APP := apps/huff.cpp

SOURCES := src/FrequencyTable.cpp \
           src/HuffmanTree.cpp \
           src/CompressionNaive.cpp \
           src/BitWriter.cpp

HEADERS := include/huffman/FrequencyTable.hpp \
           include/huffman/HuffmanTree.hpp \
           include/huffman/CompressionNaive.hpp \
           include/huffman/BitWriterNaive.hpp

TESTS := tests/testnaive.cpp

# Override at the command line when profiling a different input:
# make perf-stat ARGS="compress path/to/input.txt output.huff"
ARGS ?= compress input.txt output.huff

PERF_DATA := build/perf.data
PERF_STAT_EVENTS := task-clock,context-switches,cpu-migrations,minor-faults,major-faults

build/huff: $(APP) $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(APP) $(SOURCES) -o build/huff

build/testnaive: $(TESTS) $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(TESTS) $(SOURCES) -o build/testnaive

build: build/huff

run: build/huff
	./build/huff $(ARGS)

tests: build/testnaive
	mkdir -p build/test_output
	./build/testnaive

# VM-compatible summary counters. Hardware counters are not exposed by UTM.
perf-stat: build/huff
	perf stat -r 10 \
		-e $(PERF_STAT_EVENTS) \
		-- ./build/huff $(ARGS) > /dev/null

# Sample CPU time and collect call stacks into build/perf.data.
perf-record: build/huff
	perf record \
		-e cpu-clock:u \
		-F 999 \
		-g \
		--call-graph fp \
		-o $(PERF_DATA) \
		-- ./build/huff $(ARGS) > /dev/null

# Open the interactive hotspot report from the most recent recording.
perf-report: $(PERF_DATA)
	perf report -i $(PERF_DATA)

# Print the hotspot report directly in the terminal.
perf-report-stdio: $(PERF_DATA)
	perf report --stdio -i $(PERF_DATA)

# Inspect sampled instructions/source for hot functions.
perf-annotate: $(PERF_DATA)
	perf annotate -i $(PERF_DATA)

# Generate perf.data when a report is requested and none exists.
$(PERF_DATA): build/huff
	$(MAKE) perf-record ARGS="$(ARGS)"

clean:
	rm -rf build

.PHONY: build run tests perf-stat perf-record perf-report \
        perf-report-stdio perf-annotate clean