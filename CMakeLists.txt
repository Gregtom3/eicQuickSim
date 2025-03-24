cmake_minimum_required(VERSION 3.10)
project(eicQuickSim)

# Find ROOT. eic-shell should provide it automatically.
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

# Manually set the path to your HepMC3 library directory
# (adjust the directory name if it has a unique hash or version suffix)
set(HEPMC3_LIB_DIR "/opt/software/linux-debian12-x86_64_v2/gcc-12.2.0/hepmc3-3.3.0-km7myoz2ff4yelux5c35wx3yhmaiggvj/lib")

include_directories("${HEPMC3_LIB_DIR}/../include")  # "include" is one level above "lib"

# Build the test
add_executable(test00_readData src/tests/test00_readData.C)

# Link libs:
# 1) ROOT libraries
# 2) main HepMC3 library
# 3) the separate HepMC3 rootIO library (libHepMC3rootIO.so)
target_link_libraries(
    test00_readData
    PRIVATE
        ${ROOT_LIBRARIES}
        "${HEPMC3_LIB_DIR}/libHepMC3.so"
        "${HEPMC3_LIB_DIR}/libHepMC3rootIO.so"
)

# Enable testing and register test
enable_testing()
add_test(
    NAME readDataTest
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test00_readData
)
