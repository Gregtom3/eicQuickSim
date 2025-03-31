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
    auto scatElectrons = searchParticle(evt, 1, 11);
    // Find target hadron (pIn): status==4, pid==2112.
    auto initHadrons = searchParticle(evt, 4, 2112);
    if(initHadrons.empty()){ // try proton
        initHadrons = searchParticle(evt, 4, 2212);
    }
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
    return (pIn * h) / (pIn * q);
}

double Kinematics::eta(const TLorentzVector& h){
    return h.PseudoRapidity();
}

double Kinematics::phi(const TLorentzVector& q, const TLorentzVector& h, const TLorentzVector& eIn){
    TVector3 q3(q.Px(), q.Py(), q.Pz());
    TVector3 l3(eIn.Px(), eIn.Py(), eIn.Pz());
    TVector3 h3(h.Px(), h.Py(), h.Pz());

    TVector3 qcrossl = q3.Cross(l3);
    TVector3 qcrossh = q3.Cross(h3);

    double factor1 = (qcrossl * h3) / std::abs(qcrossl * h3);
    double factor2 = (qcrossl * qcrossh) / (qcrossl.Mag() * qcrossh.Mag());
    return factor1 * acos(factor2);
}

double Kinematics::pT_lab(const TLorentzVector& h){
    return h.Pt();
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
    // Ensure DIS has been computed.
    if (disKin_.Q2 <= 0) {
        std::cerr << "Kinematics::computeSIDIS: DIS kinematics not computed properly." << std::endl;
        return;
    }

    // Clear previous SIDIS values.
    sidisKin_.xF.clear();
    sidisKin_.eta.clear();
    sidisKin_.z.clear();
    sidisKin_.phi.clear();
    sidisKin_.pT_lab.clear();
    sidisKin_.pT_com.clear();

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

// New method: computeDISIDS for dihadron kinematics.
void Kinematics::computeDISIDS(const HepMC3::GenEvent& evt, int pid1, int pid2) {
    // Ensure DIS has been computed.
    if (disKin_.Q2 <= 0) {
        std::cerr << "Kinematics::computeDISIDS: DIS kinematics not computed properly." << std::endl;
        return;
    }
    
    // Clear any previous dihadron kinematics.
    dihadKin_.clear();
    
    std::vector<std::shared_ptr<const HepMC3::GenParticle>> particles1 = searchParticle(evt, 1, pid1);
    std::vector<std::shared_ptr<const HepMC3::GenParticle>> particles2;
    
    // If both PIDs are the same, use one list and form unique pairs.
    bool samePID = (pid1 == pid2);
    if (samePID) {
        particles2 = particles1;
    } else {
        particles2 = searchParticle(evt, 1, pid2);
    }
    
    // Form unique pairs.
    if (samePID) {
        for (size_t i = 0; i < particles1.size(); ++i) {
            for (size_t j = i+1; j < particles1.size(); ++j) {
                TLorentzVector p1 = buildFourVector(particles1[i]);
                TLorentzVector p2 = buildFourVector(particles1[j]);
                dihadronKinematics dih;
                
                // Individual hadron kinematics.
                dih.z1 = z(disKin_.q, p1, disKin_.pIn);
                dih.z2 = z(disKin_.q, p2, disKin_.pIn);
                dih.pT_lab_1 = pT_lab(p1);
                dih.pT_lab_2 = pT_lab(p2);
                dih.pT_com_1 = pT_com(disKin_.q, p1, disKin_.pIn);
                dih.pT_com_2 = pT_com(disKin_.q, p2, disKin_.pIn);
                dih.xF1 = xF(disKin_.q, p1, disKin_.pIn, disKin_.W);
                dih.xF2 = xF(disKin_.q, p2, disKin_.pIn, disKin_.W);
                
                // Pair kinematics.
                TLorentzVector pair = p1 + p2;
                dih.z_pair = z(disKin_.q, pair, disKin_.pIn);
                dih.phi_h = phi(disKin_.q, pair, disKin_.eIn);
                dih.phi_R_method0 = phi_R(disKin_.q, disKin_.eIn, p1, p2, 0);
                dih.phi_R_method1 = phi_R(disKin_.q, disKin_.eIn, p1, p2, 1);
                dih.pT_lab_pair = pT_lab(pair);
                dih.pT_com_pair = pT_com(disKin_.q, pair, disKin_.pIn);
                dih.xF_pair = xF(disKin_.q, pair, disKin_.pIn, disKin_.W);
                // New: compute com_th and invariant mass.
                dih.com_th = com_th(p1, p2);
                dih.Mh = invariantMass(p1, p2);
                
                dihadKin_.push_back(dih);
            }
        }
    } else {
        // Different PIDs: form all combinations.
        for (size_t i = 0; i < particles1.size(); ++i) {
            for (size_t j = 0; j < particles2.size(); ++j) {
                TLorentzVector p1 = buildFourVector(particles1[i]);
                TLorentzVector p2 = buildFourVector(particles2[j]);
                dihadronKinematics dih;
                
                // Individual kinematics.
                dih.z1 = z(disKin_.q, p1, disKin_.pIn);
                dih.z2 = z(disKin_.q, p2, disKin_.pIn);
                dih.pT_lab_1 = pT_lab(p1);
                dih.pT_lab_2 = pT_lab(p2);
                dih.pT_com_1 = pT_com(disKin_.q, p1, disKin_.pIn);
                dih.pT_com_2 = pT_com(disKin_.q, p2, disKin_.pIn);
                dih.xF1 = xF(disKin_.q, p1, disKin_.pIn, disKin_.W);
                dih.xF2 = xF(disKin_.q, p2, disKin_.pIn, disKin_.W);
                
                // Pair kinematics.
                TLorentzVector pair = p1 + p2;
                dih.z_pair = z(disKin_.q, pair, disKin_.pIn);
                dih.phi_h = phi(disKin_.q, pair, disKin_.eIn);
                dih.phi_R_method0 = phi_R(disKin_.q, disKin_.eIn, p1, p2, 0);
                dih.phi_R_method1 = phi_R(disKin_.q, disKin_.eIn, p1, p2, 1);
                dih.pT_lab_pair = pT_lab(pair);
                dih.pT_com_pair = pT_com(disKin_.q, pair, disKin_.pIn);
                dih.xF_pair = xF(disKin_.q, pair, disKin_.pIn, disKin_.W);
                // New: compute com_th and invariant mass.
                dih.com_th = com_th(p1, p2);
                dih.Mh = invariantMass(p1, p2);
                
                dihadKin_.push_back(dih);
            }
        }
    }
}

// Static function to calculate phi_R for a dihadron.
double Kinematics::phi_R(const TLorentzVector& Q, const TLorentzVector& L, 
                           const TLorentzVector& p1, const TLorentzVector& p2, int method) {
    TLorentzVector ph = p1 + p2;
    TLorentzVector r = 0.5 * (p1 - p2);

    TVector3 q(Q.Px(), Q.Py(), Q.Pz());
    TVector3 l(L.Px(), L.Py(), L.Pz());
    TVector3 R(r.Px(), r.Py(), r.Pz());

    TVector3 Rperp;
    // Define Rperp according to the chosen method.
    switch(method){
    case 0:   // HERMES 0803.2367 angle "RT"
        Rperp = R - (q * R) / (q * q) * q;
        break;
    case 1: { // Using Matevosyan et al. 1707.04999 to obtain R_perp.
        TLorentzVector init_target;
        init_target.SetPxPyPzE(0, 0, 0, 0.938272);
        double z1 = (init_target * p1) / (init_target * Q);
        double z2 = (init_target * p2) / (init_target * Q);
        TVector3 P1(p1.Px(), p1.Py(), p1.Pz());
        TVector3 P2(p2.Px(), p2.Py(), p2.Pz());
        TVector3 P1perp = P1 - (q * P1) / (q * q) * q;
        TVector3 P2perp = P2 - (q * P2) / (q * q) * q;
        Rperp = (z2 * P1perp - z1 * P2perp) * (1/(z1 + z2));
        break;
    }
    default:
        Rperp = R; // Fallback
        break;
    }

    TVector3 qcrossl = q.Cross(l);
    TVector3 qcrossRperp = q.Cross(Rperp);

    double factor1 = (qcrossl * Rperp) / std::abs(qcrossl * Rperp);
    double factor2 = (qcrossl * qcrossRperp) / (qcrossl.Mag() * qcrossRperp.Mag());

    return factor1 * acos(factor2);
}

// New static method: compute the com_th (center-of-mass polar angle) of the pair.
double Kinematics::com_th(const TLorentzVector& P1, const TLorentzVector& P2) {
    TLorentzVector Ptotal = P1 + P2;
    TVector3 comBOOST = Ptotal.BoostVector();
    TLorentzVector P1_copy = P1;  // work on a copy to preserve original
    P1_copy.Boost(-comBOOST);
    // Following the old implementation: return the angle between the boosted P1 and the boost vector.
    return P1_copy.Angle(comBOOST);
}

// New static method: compute the invariant mass of the pair.
double Kinematics::invariantMass(const TLorentzVector& P1, const TLorentzVector& P2) {
    return (P1 + P2).M();
}

disKinematics Kinematics::getDISKinematics() const {
    return disKin_;
}

sidisKinematics Kinematics::getSIDISKinematics() const {
    return sidisKin_;
}

std::vector<dihadronKinematics> Kinematics::getDISIDSKinematics() const {
    return dihadKin_;
}

} // namespace eicQuickSim
