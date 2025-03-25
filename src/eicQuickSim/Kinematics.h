#ifndef KINEMATICS_H
#define KINEMATICS_H

#include "HepMC3/GenEvent.h"
#include "TLorentzVector.h"
#include <vector>
#include <memory>

namespace eicQuickSim {

// Structure to hold DIS kinematics
struct disKinematics {
    double Q2;  // momentum transfer squared (negative)
    double x;   // Bjorken x
    double W;   // invariant mass of the hadronic system
};

class Kinematics {
public:
    // Compute DIS kinematics from a GenEvent.
    static disKinematics computeDIS(const HepMC3::GenEvent& evt);

    // Build a TLorentzVector from a particle.
    static TLorentzVector buildFourVector(const std::shared_ptr<const HepMC3::GenParticle>& particle);

    // Search for particles with a given status and pid.
    static std::vector<std::shared_ptr<const HepMC3::GenParticle>> searchParticle(const HepMC3::GenEvent& evt, int status, int pid);
};

} // namespace eicQuickSim

#endif // KINEMATICS_H
