.PHONY: install all build run clean tests test00 test01 test02 test03

BUILD_DIR = build
TESTS = test00 test01 test02 test03

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
