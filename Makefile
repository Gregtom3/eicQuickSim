.PHONY: install all build run clean tests \
        test00_readData test01_grabFiles test02_dataSummary test03_weightHistDIS \
        test04_printEvent test05_weightHistSIDIS test06_uploadCSV test07_migrationReader yaml-cpp \
		test08_migrationHist

BUILD_DIR = build
TESTS = test00_readData test01_grabFiles test02_dataSummary test03_weightHistDIS \
        test04_printEvent test05_weightHistSIDIS test06_uploadCSV test07_migrationReader \
		test08_migrationHist

# Setup
install:
	pip install -r requirements.txt

# Build everything including yaml-cpp
build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make

# Run specific tests
$(TESTS): build
	./$(BUILD_DIR)/$@

# Run all tests
tests: build
	cd $(BUILD_DIR) && ctest --verbose

# Clean up build
clean:
	rm -rf $(BUILD_DIR)

# Default: install and build
all: install build
