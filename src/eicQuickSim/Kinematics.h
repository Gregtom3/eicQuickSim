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
    std::vector<double> xF;     // Feynman xF
    std::vector<double> eta;    // pseudorapidity
    std::vector<double> z;      // z (energy fraction)
    std::vector<double> phi;    // azimuthal angle (w.r.t gamma-N COM frame)
    std::vector<double> pT_lab; // transverse momentum (w.r.t lab frame)
    std::vector<double> pT_com; // transverse momentum (w.r.t gamma-N COM frame)
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
   
    // Calculate eta for a hadron
    // h: hadron four-vector
    static double eta(const TLorentzVector& h);

    // Calculate z for a hadron
    // q: virtual photon, h: hadron four-vector, pIn: nucleon target
    static double z(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& pIn);

    // Calculate azimuthal phi for a hadron (gamma-N COM frame)
    // q: virtual photon, h: hadron four-vector, eIn: electron beam
    static double phi(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& eIn);

    // Calculate pT for a hadron (lab frame)
    static double pT_lab(const TLorentzVector& h);

    // Calculate pT for a hadron (gamma-N COM frame)
    // q: virtual photon, h: hadron four-vector, pIn: nucleon target
    static double pT_com(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& pIn);
private:
    disKinematics disKin_;
    sidisKinematics sidisKin_;
};

} // namespace eicQuickSim

#endif // KINEMATICS_H
