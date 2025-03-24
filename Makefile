.PHONY: install all build run clean tests

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

# Run the test executable manually (if desired)
run: build
	./$(BUILD_DIR)/test00_readData

# Clean the build directory
clean:
	rm -rf $(BUILD_DIR)

# Build all (Python + C++)
all: install build

# Run CTest-based tests
tests: build
	cd $(BUILD_DIR) && ctest --verbose
