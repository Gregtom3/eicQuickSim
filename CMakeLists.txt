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
add_subdirectory(${YAML_CPP_SOURCE_DIR} ${YAML_CPP_BUILD_DIR} EXCLUDE_FROM_ALL)
include_directories("${YAML_CPP_SOURCE_DIR}/include")
include_directories("${YAML_CPP_BUILD_DIR}/include")
link_directories("${YAML_CPP_BUILD_DIR}")
set(YAML_CPP_LIBRARIES yaml-cpp)

# ---------------------------------------------------------------------
# Define minimal common sources (from src/eicQuickSim) for individual tests.
# These are the sources for the common components (e.g. FileManager, Weights, etc.)
set(EIC_FileManager ${CMAKE_SOURCE_DIR}/src/eicQuickSim/FileManager.C)
set(EIC_Weights ${CMAKE_SOURCE_DIR}/src/eicQuickSim/Weights.C)
set(EIC_Kinematics ${CMAKE_SOURCE_DIR}/src/eicQuickSim/Kinematics.C)
set(EIC_BinningScheme ${CMAKE_SOURCE_DIR}/src/eicQuickSim/BinningScheme.C)
set(EIC_CombinedRowsProcessor ${CMAKE_SOURCE_DIR}/src/eicQuickSim/CombinedRowsProcessor.C)
set(EIC_Analysis ${CMAKE_SOURCE_DIR}/src/eicQuickSim/Analysis.C)
set(EIC_TreeManager ${CMAKE_SOURCE_DIR}/src/eicQuickSim/TreeManager.C)

# ---------------------------------------------------------------------
# Build shared library for use in ROOT macros
set(EIC_ALL_SOURCES
    ${EIC_FileManager}
    ${EIC_Weights}
    ${EIC_Kinematics}
    ${EIC_BinningScheme}
    ${EIC_CombinedRowsProcessor}
    ${EIC_Analysis}
    ${EIC_TreeManager}
)

add_library(eicQuickSim SHARED ${EIC_ALL_SOURCES})
target_include_directories(eicQuickSim PUBLIC ${CMAKE_SOURCE_DIR}/src/eicQuickSim)
target_link_libraries(eicQuickSim PUBLIC ${ROOT_LIBRARIES} yaml-cpp
    "${HEPMC3_LIB_DIR}/libHepMC3.so" "${HEPMC3_LIB_DIR}/libHepMC3rootIO.so")

set_target_properties(eicQuickSim PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

install(FILES $<TARGET_FILE:eicQuickSim> DESTINATION lib)

# ---------------------------------------------------------------------
# Define a function for adding a test executable with minimal sources.
# The caller passes the test source file and any additional required sources.
function(add_eic_test_minimal targetName testSource)
    # All additional source arguments are forwarded to add_executable.
    add_executable(${targetName} ${ARGN} ${testSource})
    target_link_libraries(${targetName} PRIVATE ${ROOT_LIBRARIES} yaml-cpp
        "${HEPMC3_LIB_DIR}/libHepMC3.so" "${HEPMC3_LIB_DIR}/libHepMC3rootIO.so")
    add_test(NAME ${targetName}Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${targetName})
endfunction()

# ---------------------------------------------------------------------
# Register tests
add_eic_test_minimal(test00_readData "src/tests/test00_readData.C" ${EIC_Kinematics})
add_eic_test_minimal(test01_grabFiles "src/tests/test01_grabFiles.C" ${EIC_FileManager})
add_eic_test_minimal(test02_dataSummary "src/tests/test02_dataSummary.C" ${EIC_FileManager} ${EIC_Weights})
add_eic_test_minimal(test03_weightHistDIS "src/tests/test03_weightHistDIS.C" ${EIC_FileManager} ${EIC_Weights} ${EIC_Kinematics})
add_eic_test_minimal(test03_1_weightHistDIS "src/tests/test03_1_weightHistDIS.C" ${EIC_FileManager} ${EIC_Weights} ${EIC_Kinematics})
add_eic_test_minimal(test04_printEvent "src/tests/test04_printEvent.C")
add_eic_test_minimal(test05_weightHistSIDIS "src/tests/test05_weightHistSIDIS.C" ${EIC_FileManager} ${EIC_Weights} ${EIC_Kinematics} ${EIC_BinningScheme})
add_eic_test_minimal(test06_uploadCSV "src/tests/test06_uploadCSV.C" ${EIC_FileManager} ${EIC_Weights})
add_eic_test_minimal(test07_epNevents "src/tests/test07_epNevents.C" ${EIC_FileManager} ${EIC_Weights} ${EIC_BinningScheme} ${EIC_Kinematics})
add_eic_test_minimal(test08_weightHistDISIDIS "src/tests/test08_weightHistDISIDIS.C" ${EIC_FileManager} ${EIC_Weights} ${EIC_Kinematics} ${EIC_BinningScheme})

enable_testing()
