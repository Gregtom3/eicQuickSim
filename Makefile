.PHONY: install all build run clean tests test00 test01

# Directory for the CMake build
BUILD_DIR = build

# NOTE: This Makefile expects to be run inside the eic-shell environment,
# which provides ROOT, HepMC3, etc.

# Install Python requirements
install:
	pip install -r requirements.txt

# Configure and build with CMake
build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make

# Run the old test00_readData
test00: build
	./$(BUILD_DIR)/test00_readData

# Run the new test01_grabFiles
test01: build
	./$(BUILD_DIR)/test01_grabFiles

# Run all CTest-based tests (both test00 and test01, plus any others)
tests: build
	cd $(BUILD_DIR) && ctest --verbose

# Just a convenience target if you want to run one of them quickly
run: test01

# Clean the build directory
clean:
	rm -rf $(BUILD_DIR)

# Build everything (Python + C++):
all: install build
