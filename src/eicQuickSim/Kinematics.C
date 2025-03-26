#include "Kinematics.h"
#include <cmath>
#include <iostream>
#include "HepMC3/GenParticle.h"
#include "TLorentzVector.h"
#include "TVector3.h"

namespace eicQuickSim {

Kinematics::Kinematics() {
    disKin_.Q2 = 0;
    disKin_.x  = 0;
    disKin_.W  = 0;
}

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

void Kinematics::computeDIS(const HepMC3::GenEvent& evt) {
    // Find initial electron (eIn): status==4, pid==11.
    auto initElectrons = searchParticle(evt, 4, 11);
    // Find scattered electron (eOut): status==21, pid==11.
    auto scatElectrons = searchParticle(evt, 21, 11);
    // Find target hadron (pIn): status==4, pid==2112.
    auto initHadrons = searchParticle(evt, 4, 2112);

    if (initElectrons.empty() || scatElectrons.empty() || initHadrons.empty()) {
        std::cerr << "Kinematics::computeDIS: Required DIS particle(s) not found." << std::endl;
        return;
    }

    // Use the first matching particle in each case.
    disKin_.eIn  = buildFourVector(initElectrons[0]);
    disKin_.eOut = buildFourVector(scatElectrons[0]);
    disKin_.pIn  = buildFourVector(initHadrons[0]);
    disKin_.q    = disKin_.eIn - disKin_.eOut;
    disKin_.Q2   = -disKin_.q.M2();

    double denominator = 2 * (disKin_.pIn.E() * disKin_.q.E() - disKin_.pIn.Pz() * disKin_.q.Pz());
    disKin_.x = (denominator != 0.0) ? disKin_.Q2 / denominator : 0.0;

    double W2 = (disKin_.pIn + disKin_.q).M2();
    disKin_.W = (W2 > 0.0) ? std::sqrt(W2) : 0.0;
}

double Kinematics::xF(const TLorentzVector& q, const TLorentzVector& h,
                      const TLorentzVector& pIn, double W) {
    TLorentzVector com = q + pIn;
    TVector3 comBOOST = com.BoostVector();
    TLorentzVector qq = q;
    TLorentzVector hh = h;
    qq.Boost(-comBOOST);
    hh.Boost(-comBOOST);
    double mag_qq = qq.Vect().Mag();
    if (mag_qq == 0 || W == 0) return 0;
    return 2 * (qq.Vect().Dot(hh.Vect())) / (mag_qq * W);
}

double Kinematics::z(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& pIn){
    return (pIn*h)/(pIn*q);
}

double Kinematics::phi(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& eIn){

    TVector3 q3(q.Px(), q.Py(), q.Pz());
    TVector3 l3(eIn.Px(), eIn.Py(), eIn.Pz());
    TVector3 h3(h.Px(), h.Py(), h.Pz());

    TVector3 qcrossl = q3.Cross(l3);
    TVector3 qcrossh = q3.Cross(h3);

    double factor1 = (qcrossl*h3)/std::abs(qcrossl*h3);
    double factor2 = (qcrossl*qcrossh)/qcrossl.Mag()/qcrossh.Mag();
    return factor1*acos(factor2);
}

double Kinematics::pT_lab(const TLorentzVector& h){
    return h.Pt();
}

double Kinematics::eta(const TLorentzVector& h){
    return h.PseudoRapidity();
}

double Kinematics::pT_com(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& pIn){
    TLorentzVector com = q + pIn;
    TVector3 comBOOST = com.BoostVector();
    TLorentzVector qq = q;
    TLorentzVector hh = h;
    qq.Boost(-comBOOST);
    hh.Boost(-comBOOST);
    return hh.Pt(qq.Vect());
}

void Kinematics::computeSIDIS(const HepMC3::GenEvent& evt, int pid) {
    // Make sure DIS has been computed.
    if (disKin_.Q2 <= 0) {
        std::cerr << "Kinematics::computeSIDIS: DIS kinematics not computed properly." << std::endl;
        return;
    }

    // Clear previous SIDIS values.
    sidisKin_.xF.clear();

    // Look for final state hadrons: status==1 and the given pid.
    auto finalParticles = searchParticle(evt, 1, pid);
    for (auto& particle : finalParticles) {
        TLorentzVector hadron = buildFourVector(particle);
        double xf_val = xF(disKin_.q, hadron, disKin_.pIn, disKin_.W);
        double eta_val = eta(hadron);
        double z_val = z(disKin_.q, hadron, disKin_.pIn);
        double phi_val = phi(disKin_.q, hadron, disKin_.eIn);
        double pT_lab_val = pT_lab(hadron);
        double pT_com_val = pT_com(disKin_.q, hadron, disKin_.pIn);
        sidisKin_.xF.push_back(xf_val);
        sidisKin_.eta.push_back(eta_val);
        sidisKin_.z.push_back(z_val);
        sidisKin_.phi.push_back(phi_val);
        sidisKin_.pT_lab.push_back(pT_lab_val);
        sidisKin_.pT_com.push_back(pT_com_val);
    }
}

disKinematics Kinematics::getDISKinematics() const {
    return disKin_;
}

sidisKinematics Kinematics::getSIDISKinematics() const {
    return sidisKin_;
}

} // namespace eicQuickSim
