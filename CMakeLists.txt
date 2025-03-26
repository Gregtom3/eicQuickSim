cmake_minimum_required(VERSION 3.10)
project(eicQuickSim)

# ROOT and HepMC3 setup
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

set(HEPMC3_LIB_DIR "/opt/software/linux-debian12-x86_64_v2/gcc-12.2.0/hepmc3-3.3.0-km7myoz2ff4yelux5c35wx3yhmaiggvj/lib")
include_directories("${HEPMC3_LIB_DIR}/../include")
include_directories(${CMAKE_SOURCE_DIR}/src/eicQuickSim)

# Macro for adding test executables
function(add_eic_test name sources use_hepmc)
    add_executable(${name} ${sources})
    target_link_libraries(${name} PRIVATE ${ROOT_LIBRARIES})
    if (${use_hepmc})
        target_link_libraries(${name} PRIVATE
            "${HEPMC3_LIB_DIR}/libHepMC3.so"
            "${HEPMC3_LIB_DIR}/libHepMC3rootIO.so"
        )
    endif()
    add_test(NAME ${name}Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${name})
endfunction()

# Register tests
add_eic_test(test00_readData      "src/tests/test00_readData.C"         TRUE)
add_eic_test(test01_grabFiles     "src/tests/test01_grabFiles.C;src/eicQuickSim/FileManager.C" FALSE)
add_eic_test(test02_dataSummary   "src/tests/test02_dataSummary.C;src/eicQuickSim/FileManager.C;src/eicQuickSim/FileDataSummary.C" FALSE)
add_eic_test(test03_weightHistDIS    
    "src/tests/test03_weightHistDIS.C;src/eicQuickSim/FileManager.C;src/eicQuickSim/FileDataSummary.C;src/eicQuickSim/Kinematics.C" 
    TRUE)
add_eic_test(test04_printEvent    
    "src/tests/test04_printEvent.C;src/eicQuickSim/FileManager.C;src/eicQuickSim/FileDataSummary.C;src/eicQuickSim/Kinematics.C" 
    TRUE)
add_eic_test(test05_weightHistSIDIS    
    "src/tests/test05_weightHistSIDIS.C;src/eicQuickSim/FileManager.C;src/eicQuickSim/FileDataSummary.C;src/eicQuickSim/Kinematics.C" 
    TRUE)
add_eic_test(test06_uploadCSV    
    "src/tests/test06_uploadCSV.C;src/eicQuickSim/FileManager.C;src/eicQuickSim/FileDataSummary.C;src/eicQuickSim/Kinematics.C" 
    TRUE)
enable_testing()