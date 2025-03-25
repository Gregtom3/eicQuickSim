#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderRootTree.h"
#include "HepMC3/GenParticle.h"

#include <iostream>
#include <iomanip>
#include <string>

using namespace HepMC3;

int main() {
    // Filename definition
    std::string filename = "root://dtn-eic.jlab.org//volatile/eic/EPIC/EVGEN/SIDIS/pythia6-eic/1.1.0/en_noradcor/10x100/q2_1000to100000/pythia6-eic_1.1.0_en_noradcor_10x100_q2_1000_100000_run001.ab.hepmc3.tree.root";

    // Open the file using ROOT's ReaderRootTree.
    ReaderRootTree reader(filename);
    if (reader.failed()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 1;
    }
    
    // Read one event from the file.
    GenEvent evt;
    reader.read_event(evt);
    if (reader.failed()) {
        std::cerr << "Failed to read event from file: " << filename << std::endl;
        return 1;
    }
    
    // Print header for the output table.
    std::cout << std::setw(5)  << "Row"
              << std::setw(8)  << "PID"
              << std::setw(10) << "Status"
              << std::setw(12) << "px"
              << std::setw(12) << "py"
              << std::setw(12) << "pz"
              << std::setw(12) << "e" 
              << std::endl;
    
    // Loop over each particle in the event and print its details.
    int row = 0;
    for (const auto &particle : evt.particles()) {
        std::cout << std::setw(5)  << row
                  << std::setw(8)  << particle->pid()
                  << std::setw(10) << particle->status()
                  << std::setw(12) << particle->momentum().px()
                  << std::setw(12) << particle->momentum().py()
                  << std::setw(12) << particle->momentum().pz()
                  << std::setw(12) << particle->momentum().e()
                  << std::endl;
        row++;
    }
    
    reader.close();
    return 0;
}
