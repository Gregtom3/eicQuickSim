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

// Structure to hold SIDIS kinematics (one per final state hadron).
struct sidisKinematics {
    double Q2;           // -q^2
    double x;            // Bjorken x
    double xF;     // Feynman xF
    double eta;    // pseudorapidity
    double z;      // energy fraction
    double phi;    // azimuthal angle (in gamma-N COM frame)
    double pT_lab; // transverse momentum (lab frame)
    double pT_com; // transverse momentum (gamma-N COM frame)
};

// Structure to hold dihadron kinematics for DISIDIS.
struct dihadronKinematics {
    double Q2;           // -q^2
    double x;            // Bjorken x
    // Pair-level quantities.
    double z_pair;
    double phi_h;
    double phi_R_method0;
    double phi_R_method1;
    double pT_lab_pair;
    double pT_com_pair;
    double xF_pair;
    double com_th; // center-of-mass polar angle of the pair
    double Mh;     // invariant mass of the pair
    // Individual hadron quantities.
    double z1, z2;
    double pT_lab_1, pT_lab_2;
    double pT_com_1, pT_com_2;
    double xF1, xF2;
};

class Kinematics {
public:
    Kinematics();

    // Compute and store DIS kinematics from a GenEvent.
    void computeDIS(const HepMC3::GenEvent& evt);

    // Compute SIDIS kinematics for a given final state particle (identified by pid).
    // The results are stored in a vector of sidisKinematics.
    void computeSIDIS(const HepMC3::GenEvent& evt, int pid);

    // Compute dihadron kinematics for DISIDIS.
    // Requires two PIDs; if they are identical, unique pairs are formed.
    void computeDISIDS(const HepMC3::GenEvent& evt, int pid1, int pid2);

    // Getters for the stored kinematics.
    disKinematics getDISKinematics() const;
    std::vector<sidisKinematics> getSIDISKinematics() const;
    std::vector<dihadronKinematics> getDISIDSKinematics() const;

    // Helper functions.
    static TLorentzVector buildFourVector(const std::shared_ptr<const HepMC3::GenParticle>& particle);
    static std::vector<std::shared_ptr<const HepMC3::GenParticle>>
        searchParticle(const HepMC3::GenEvent& evt, int status, int pid);

    // Calculate xF for a hadron.
    static double xF(const TLorentzVector& q, const TLorentzVector& h,
                     const TLorentzVector& pIn, double W);
   
    // Calculate eta for a hadron.
    static double eta(const TLorentzVector& h);

    // Calculate z for a hadron.
    static double z(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& pIn);

    // Calculate phi (azimuthal angle in gamma-N COM frame).
    static double phi(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& eIn);

    // Calculate pT in the lab frame.
    static double pT_lab(const TLorentzVector& h);

    // Calculate pT in the gamma-N COM frame.
    static double pT_com(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& pIn);

    // Calculate phi_R for a dihadron.
    // Q: virtual photon, L: electron beam, p1, p2: hadron four-vectors, init_target: target
    // method: 0 for "RT", 1 for "R_perp".
    static double phi_R(const TLorentzVector& Q, const TLorentzVector& L, 
                        const TLorentzVector& p1, const TLorentzVector& p2, 
                        const TLorentzVector& init_target, int method);

    // New static methods for dihadron pair.
    // Compute the center-of-mass polar angle (com_th) of the pair.
    static double com_th(const TLorentzVector& P1, const TLorentzVector& P2);
    // Compute the invariant mass (Mh) of the pair.
    static double invariantMass(const TLorentzVector& P1, const TLorentzVector& P2);

private:
    disKinematics disKin_;
    std::vector<sidisKinematics> sidisKin_;
    std::vector<dihadronKinematics> dihadKin_;
};

} // namespace eicQuickSim

#endif // KINEMATICS_H
