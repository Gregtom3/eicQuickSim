.PHONY: install all build run clean tests \
        test00_readData test01_grabFiles test02_dataSummary test03_weightHistDIS \
        test04_printEvent test05_weightHistSIDIS test06_uploadCSV test07_migrationReader test08_migrationHist yaml-cpp analysis_epDIS

BUILD_DIR = build
TESTS = test00_readData test01_grabFiles test02_dataSummary test03_weightHistDIS \
        test04_printEvent test05_weightHistSIDIS test06_uploadCSV test07_migrationReader \
		test08_migrationHist

# Setup: Install Python requirements
install_requirements:
	pip install -r requirements.txt

# Build everything including yaml-cpp
build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_INSTALL_PREFIX=$(PWD)/build .. && make

# CMake install target to install executables (including analysis_epDIS)
install_cmake: build
	cd $(BUILD_DIR) && make install

# Combined install target: setup Python requirements then run CMake install
install: install_requirements install_cmake

# Run specific tests
$(TESTS): build
	./$(BUILD_DIR)/$@

# Run the analysis_epDIS executable
analysis_epDIS: build
	./$(BUILD_DIR)/analysis_epDIS

# Run all tests
tests: build
	cd $(BUILD_DIR) && ctest --verbose

# Clean up build
clean:
	rm -rf $(BUILD_DIR)

# Default: install and build
all: install build
