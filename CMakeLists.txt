cmake_minimum_required(VERSION 3.10)
project(eicQuickSim)

# ROOT and HepMC3 setup
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

set(HEPMC3_LIB_DIR "/opt/software/linux-debian12-x86_64_v2/gcc-12.2.0/hepmc3-3.3.0-km7myoz2ff4yelux5c35wx3yhmaiggvj/lib")
include_directories("${HEPMC3_LIB_DIR}/../include")
include_directories(${CMAKE_SOURCE_DIR}/src/eicQuickSim)

# Build yaml-cpp as a submodule if not already built
set(YAML_CPP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/yaml-cpp")
set(YAML_CPP_BUILD_DIR "${CMAKE_BINARY_DIR}/third_party/yaml-cpp_build")

# Build yaml-cpp as part of your build
add_subdirectory(${YAML_CPP_SOURCE_DIR} ${YAML_CPP_BUILD_DIR} EXCLUDE_FROM_ALL)

# Add yaml-cpp include and link directories
include_directories("${YAML_CPP_SOURCE_DIR}/include")
include_directories("${YAML_CPP_BUILD_DIR}/include")
link_directories("${YAML_CPP_BUILD_DIR}")

# Link yaml-cpp to all targets
set(YAML_CPP_LIBRARIES yaml-cpp)

# Include our source directory.
include_directories(${CMAKE_SOURCE_DIR}/src/eicQuickSim)

# Collect all common eicQuickSim sources.
file(GLOB EIC_SOURCES ${CMAKE_SOURCE_DIR}/src/eicQuickSim/*.C)

# Macro for adding test executables.
function(add_eic_test name test_sources)
    add_executable(${name} ${test_sources} ${EIC_SOURCES})
    target_link_libraries(${name} PRIVATE ${ROOT_LIBRARIES} yaml-cpp
        "${HEPMC3_LIB_DIR}/libHepMC3.so" "${HEPMC3_LIB_DIR}/libHepMC3rootIO.so")
    add_test(NAME ${name}Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${name})
endfunction()

# Register tests.
add_eic_test(test00_readData      "src/tests/test00_readData.C")
add_eic_test(test01_grabFiles     "src/tests/test01_grabFiles.C")
add_eic_test(test02_dataSummary   "src/tests/test02_dataSummary.C")
add_eic_test(test03_weightHistDIS "src/tests/test03_weightHistDIS.C")
add_eic_test(test04_printEvent    "src/tests/test04_printEvent.C")
add_eic_test(test05_weightHistSIDIS "src/tests/test05_weightHistSIDIS.C")
add_eic_test(test06_uploadCSV     "src/tests/test06_uploadCSV.C")
add_eic_test(test07_epNevents     "src/tests/test07_epNevents.C")
add_eic_test(test08_weightHistDISIDIS     "src/tests/test08_weightHistDISIDIS.C")

# New function for adding macro executables.
function(add_eic_executable targetName macroSource)
    add_executable(${targetName} ${macroSource} ${EIC_SOURCES})
    target_link_libraries(${targetName} PRIVATE ${ROOT_LIBRARIES} yaml-cpp
        "${HEPMC3_LIB_DIR}/libHepMC3.so" "${HEPMC3_LIB_DIR}/libHepMC3rootIO.so")
    install(TARGETS ${targetName} RUNTIME DESTINATION bin)
endfunction()

# Register macro executables using the new function.
add_eic_executable(analysis_DIS macros/analysis_DIS.C)
add_eic_executable(analysis_SIDIS macros/analysis_SIDIS.C)
add_eic_executable(analysis_DISIDIS macros/analysis_DISIDIS.C)
add_eic_executable(preprocess_HPC macros/preprocess_HPC.C)

enable_testing()
