#include "Kinematics.h"

#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderRootTree.h"
#include "HepMC3/GenParticle.h"

#include "TLorentzVector.h"
#include "TFile.h"
#include "TTree.h"

#include <iostream>
#include <fstream>

using namespace HepMC3;

int main() {
    // Open ROOT file 
    std::string rootfile = "root://dtn-eic.jlab.org//volatile/eic/EPIC/EVGEN/SIDIS/pythia6-eic/1.1.0/en_noradcor/10x100/q2_1000to100000/pythia6-eic_1.1.0_en_noradcor_10x100_q2_1000_100000_run001.ab.hepmc3.tree.root";
    //std::string rootfile = "root://dtn-eic.jlab.org//volatile/eic/EPIC/EVGEN/SIDIS/pythia6-eic/1.1.0/en_noradcor/10x100/q2_1to10/pythia6-eic_1.1.0_en_noradcor_10x100_q2_1_10_run043.ab.hepmc3.tree.root";
    //std::string rootfile = "root://dtn-eic.jlab.org//volatile/eic/EPIC/EVGEN/DIS/NC/10x100/minQ2=1/pythia8NCDIS_10x100_minQ2=1_beamEffects_xAngle=-0.025_hiDiv_1.hepmc3.tree.root";
    ReaderRootTree root_input(rootfile);
    
    int events_parsed = 0;

    // Loop through the entire input file.
    while (!root_input.failed()) {
        GenEvent evt;
        root_input.read_event(evt);
        if (root_input.failed()) break;
        events_parsed++;
        
        // Print progress every 1000 events.
        if (events_parsed % 1000 == 0) {
            std::cout << "Processed " << events_parsed << " events." << std::endl;
        }
        
        // Initialize pointers for the three particles.
        // initial electron: status==4, pid==11
        // initial hadron (neutron): status==4, pid==2112
        // scattered electron: status==21, pid==11
        GenParticlePtr initElectron = nullptr;
        GenParticlePtr initHadron   = nullptr;
        GenParticlePtr scatElectron = nullptr;
        
        const auto& particles = evt.particles();
        for (const auto& particle : particles) {
            int status = particle->status();
            int pid = particle->pid();
            if (status == 4 && pid == 11) {
                initElectron = particle;
            } else if (status == 4 && pid == 2112) {
                initHadron = particle;
            } else if (status == 21 && pid == 11) {
                scatElectron = particle;
            }
        }

        eicQuickSim::Kinematics kin;
        kin.computeDIS(evt);
        eicQuickSim::disKinematics dis = kin.getDISKinematics();
        // Print scattered electron kinematics
        if (events_parsed < 10) {
            std::cout << "Event " << events_parsed << ": ";
            std::cout << "Q2:" << dis.Q2 << " ";
            std::cout << "x:" << dis.x << " ";
            std::cout << "W:" << dis.W << " ";
            std::cout << "Px:" << scatElectron->momentum().px() << " ";
            std::cout << "Py:" << scatElectron->momentum().py() << " ";
            std::cout << "Pz:" << scatElectron->momentum().pz() << " ";
            std::cout << "E:" << scatElectron->momentum().e() << std::endl;
            std::cout << "-------------------------------------------------------------" << std::endl;
        }
        else{
            break;
        }
        
    }
    
    root_input.close();
    
    return 0;
}
