.PHONY: install all build run clean

# Directory for CMake build
BUILD_DIR = build

# NOTE: This Makefile expects to be run inside the eic-shell environment,
# which provides the appropriate ROOT instance.

# Install Python requirements
install:
	pip install --upgrade pip && \
	pip install -r requirements.txt

# Configure and build C++ ROOT code using CMake
build:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make

# Run the compiled ROOT executable
run: build
	./$(BUILD_DIR)/main

# Clean the build directory
clean:
	rm -rf $(BUILD_DIR)

# All: install Python requirements and build C++ code
all: install build
