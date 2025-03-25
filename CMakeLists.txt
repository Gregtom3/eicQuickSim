cmake_minimum_required(VERSION 3.10)
project(eicQuickSim)

# Find ROOT. eic-shell should provide it automatically.
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

# Manually set the path to your HepMC3 library directory
set(HEPMC3_LIB_DIR "/opt/software/linux-debian12-x86_64_v2/gcc-12.2.0/hepmc3-3.3.0-km7myoz2ff4yelux5c35wx3yhmaiggvj/lib")
include_directories("${HEPMC3_LIB_DIR}/../include")  # "include" is one level above "lib"
include_directories(${CMAKE_SOURCE_DIR}/src/eicQuickSim)

# ----------------------------------------------------------------------------
# test00_readData
# ----------------------------------------------------------------------------
add_executable(test00_readData
    src/tests/test00_readData.C
)
target_link_libraries(
    test00_readData
    PRIVATE
        ${ROOT_LIBRARIES}
        "${HEPMC3_LIB_DIR}/libHepMC3.so"
        "${HEPMC3_LIB_DIR}/libHepMC3rootIO.so"
)

# ----------------------------------------------------------------------------
# test01_grabFiles
# ----------------------------------------------------------------------------
add_executable(test01_grabFiles
    src/tests/test01_grabFiles.C
    src/eicQuickSim/FileManager.C
)
target_link_libraries(
    test01_grabFiles
    PRIVATE
        ${ROOT_LIBRARIES}
)

# ----------------------------------------------------------------------------
# test02_dataSummary
# ----------------------------------------------------------------------------
add_executable(test02_dataSummary
    src/tests/test02_dataSummary.C
    src/eicQuickSim/FileManager.C
    src/eicQuickSim/FileDataSummary.C
)

target_link_libraries(test02_dataSummary PRIVATE ${ROOT_LIBRARIES})

add_test(NAME dataSummaryTest COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test02_dataSummary)

# ----------------------------------------------------------------------------
# test02_weightHist
# ----------------------------------------------------------------------------
add_executable(test03_weightHist 
    src/tests/test03_weightHist.C 
    src/eicQuickSim/FileManager.C 
    src/eicQuickSim/FileDataSummary.C
)

target_link_libraries(
    test03_weightHist 
    PRIVATE 
        ${ROOT_LIBRARIES}
        "${HEPMC3_LIB_DIR}/libHepMC3.so"
        "${HEPMC3_LIB_DIR}/libHepMC3rootIO.so"
)

add_test(NAME weightHistTest COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test03_weightHist)

# ----------------------------------------------------------------------------
# Enable CTest and define each test
# ----------------------------------------------------------------------------
enable_testing()

# test00_readData
add_test(
    NAME readDataTest
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test00_readData
)

# test01_grabFiles
add_test(
    NAME grabFilesTest
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test01_grabFiles
)

