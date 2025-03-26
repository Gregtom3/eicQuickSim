#ifndef KINEMATICS_H
#define KINEMATICS_H

#include "HepMC3/GenEvent.h"
#include "TLorentzVector.h"
#include <vector>
#include <memory>

namespace eicQuickSim {

// Structure to hold DIS kinematics.
struct disKinematics {
    TLorentzVector eIn;  // initial electron
    TLorentzVector eOut; // scattered electron
    TLorentzVector pIn;  // initial hadron (target)
    TLorentzVector q;    // virtual photon momentum (eIn - eOut)
    double Q2;           // -q^2
    double x;            // Bjorken x
    double W;            // invariant mass of hadronic system
};

// Structure to hold SIDIS kinematics. For each event there may be multiple SIDIS values.
struct sidisKinematics {
    std::vector<double> xF;  // Feynman xF values for final state hadrons
};

class Kinematics {
public:
    Kinematics();

    // Compute and store DIS kinematics from a GenEvent.
    // This fills the internal disKinematics structure.
    void computeDIS(const HepMC3::GenEvent& evt);

    // Compute SIDIS kinematics for a given final state particle (identified by pid).
    // Requires that computeDIS has already been called.
    void computeSIDIS(const HepMC3::GenEvent& evt, int pid);

    // Getters for the stored kinematics.
    disKinematics getDISKinematics() const;
    sidisKinematics getSIDISKinematics() const;

    // Helper functions:
    static TLorentzVector buildFourVector(const std::shared_ptr<const HepMC3::GenParticle>& particle);
    static std::vector<std::shared_ptr<const HepMC3::GenParticle>>
        searchParticle(const HepMC3::GenEvent& evt, int status, int pid);

    // Calculate xF for a hadron.
    // q: virtual photon, h: hadron four-vector, pIn: nucleon target, W: invariant mass.
    static double xF(const TLorentzVector& q, const TLorentzVector& h,
                     const TLorentzVector& pIn, double W);

    // Calculate z for a hadron
    // q: virtual photon, h: hadron four-vector, pIn: nucleon target
    static double z(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& pIn);
private:
    disKinematics disKin_;
    sidisKinematics sidisKin_;
};

} // namespace eicQuickSim

#endif // KINEMATICS_H
