.PHONY: install all build run clean tests test00_readData test01_grabFiles test02_dataSummary test03_weightHistDIS test04_printEvent test05_weightHistSIDIS test06_uploadCSV

BUILD_DIR = build
TESTS = test00_readData test01_grabFiles test02_dataSummary test03_weightHistDIS test04_printEvent test05_weightHistSIDIS test06_uploadCSV

# Setup
install:
	pip install -r requirements.txt

build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make

# Pattern rule for tests
$(TESTS): build
	./$(BUILD_DIR)/$@

tests: build
	cd $(BUILD_DIR) && ctest --verbose

clean:
	rm -rf $(BUILD_DIR)

all: install build
