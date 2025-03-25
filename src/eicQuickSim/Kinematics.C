#include "Kinematics.h"
#include <cmath>
#include <iostream>
#include "HepMC3/GenParticle.h"

namespace eicQuickSim {

TLorentzVector Kinematics::buildFourVector(const std::shared_ptr<const HepMC3::GenParticle>& particle) {
    TLorentzVector vec;
    vec.SetPxPyPzE(particle->momentum().px(),
                   particle->momentum().py(),
                   particle->momentum().pz(),
                   particle->momentum().e());
    return vec;
}

std::vector<std::shared_ptr<const HepMC3::GenParticle>> 
Kinematics::searchParticle(const HepMC3::GenEvent& evt, int status, int pid) {
    std::vector<std::shared_ptr<const HepMC3::GenParticle>> found;
    for (const auto& particle : evt.particles()) {
        if (particle->status() == status && particle->pid() == pid) {
            found.push_back(particle);
        }
    }
    return found;
}

disKinematics Kinematics::computeDIS(const HepMC3::GenEvent& evt) {
    disKinematics result {0.0, 0.0, 0.0};

    // Find the initial electron (eIn), scattered electron (eOut) and initial hadron (pIn).
    auto initElectrons = searchParticle(evt, 4, 11);
    auto scatElectrons = searchParticle(evt, 21, 11);
    auto initHadrons   = searchParticle(evt, 4, 2112);

    if (initElectrons.empty() || scatElectrons.empty() || initHadrons.empty()) {
        std::cerr << "Kinematics::computeDIS: Required particle(s) not found." << std::endl;
        return result;
    }

    // Use the first particle found for each type.
    auto initElectron = initElectrons[0];
    auto scatElectron = scatElectrons[0];
    auto initHadron   = initHadrons[0];

    // Build four-vectors.
    TLorentzVector eIn  = buildFourVector(initElectron);
    TLorentzVector eOut = buildFourVector(scatElectron);
    TLorentzVector pIn  = buildFourVector(initHadron);

    // Calculate the momentum transfer.
    TLorentzVector q = eIn - eOut;
    result.Q2 = -q.M2();

    // Compute Bjorken x using the initial hadron four-vector.
    double denominator = 2 * (pIn.E() * q.E() - pIn.Pz() * q.Pz());
    result.x = (denominator != 0.0) ? result.Q2 / denominator : 0.0;

    // Calculate the invariant mass W of the hadronic system.
    double W2 = (pIn + q).M2();
    result.W = (W2 > 0.0) ? std::sqrt(W2) : 0.0;

    return result;
}

} // namespace eicQuickSim
