#include "TreeManager.h"
#include <iostream>

TreeManager::TreeManager(const std::string& outputFile, const std::string& analysisType)
    : m_file(nullptr),
      m_tree(nullptr),
      m_analysisType(analysisType),
      m_weight(0.0),
      // DIS variables
      m_dis_Q2(0.0), m_dis_x(0.0), m_dis_y(0.0), m_dis_W(0.0),
      // SIDIS variables
      m_sidis_Q2(0.0), m_sidis_x(0.0), m_sidis_y(0.0), m_sidis_xF(0.0),
      m_sidis_eta(0.0), m_sidis_z(0.0), m_sidis_phi(0.0),
      m_sidis_pt_lab(0.0), m_sidis_pt_com(0.0),
      // DISIDIS variables
      m_dihad_Q2(0.0), m_dihad_x(0.0), m_dihad_y(0.0), m_dihad_z_pair(0.0),
      m_dihad_phi_h(0.0), m_dihad_phi_R_method0(0.0), m_dihad_phi_R_method1(0.0),
      m_dihad_pt_lab_pair(0.0), m_dihad_pt_com_pair(0.0),
      m_dihad_xF_pair(0.0), m_dihad_com_th(0.0), m_dihad_Mh(0.0)
{
    // Create a new TFile for output.
    m_file = new TFile(outputFile.c_str(), "RECREATE");
    if (!m_file || m_file->IsZombie()) {
        std::cerr << "Error: Could not create output file " << outputFile << std::endl;
        return;
    }

    // Create the TTree.
    m_tree = new TTree("AnalysisTree", "Tree holding analysis kinematics");

    // Setup branches based on the analysis type.
    if(m_analysisType == "DIS") {
        m_tree->Branch("Q2", &m_dis_Q2, "Q2/D");
        m_tree->Branch("x", &m_dis_x, "x/D");
        m_tree->Branch("y", &m_dis_y, "y/D");
        m_tree->Branch("W", &m_dis_W, "W/D");
        m_tree->Branch("weight", &m_weight, "weight/D");
    }
    else if(m_analysisType == "SIDIS") {
        m_tree->Branch("Q2", &m_sidis_Q2, "Q2/D");
        m_tree->Branch("x", &m_sidis_x, "x/D");
        m_tree->Branch("y", &m_sidis_y, "y/D");
        m_tree->Branch("xF", &m_sidis_xF, "xF/D");
        m_tree->Branch("eta", &m_sidis_eta, "eta/D");
        m_tree->Branch("z", &m_sidis_z, "z/D");
        m_tree->Branch("phi", &m_sidis_phi, "phi/D");
        m_tree->Branch("pt_lab", &m_sidis_pt_lab, "pt_lab/D");
        m_tree->Branch("pt_com", &m_sidis_pt_com, "pt_com/D");
        m_tree->Branch("weight", &m_weight, "weight/D");
    }
    else if(m_analysisType == "DISIDIS") {
        m_tree->Branch("Q2", &m_dihad_Q2, "Q2/D");
        m_tree->Branch("x", &m_dihad_x, "x/D");
        m_tree->Branch("y", &m_dihad_y, "y/D");
        m_tree->Branch("z_pair", &m_dihad_z_pair, "z_pair/D");
        m_tree->Branch("phi_h", &m_dihad_phi_h, "phi_h/D");
        m_tree->Branch("phi_R_method0", &m_dihad_phi_R_method0, "phi_R_method0/D");
        m_tree->Branch("phi_R_method1", &m_dihad_phi_R_method1, "phi_R_method1/D");
        m_tree->Branch("pt_lab_pair", &m_dihad_pt_lab_pair, "pt_lab_pair/D");
        m_tree->Branch("pt_com_pair", &m_dihad_pt_com_pair, "pt_com_pair/D");
        m_tree->Branch("xF_pair", &m_dihad_xF_pair, "xF_pair/D");
        m_tree->Branch("com_th", &m_dihad_com_th, "com_th/D");
        m_tree->Branch("Mh", &m_dihad_Mh, "Mh/D");
        m_tree->Branch("weight", &m_weight, "weight/D");
    }
    else {
        std::cerr << "TreeManager: Unknown analysis type '" << m_analysisType << "'" << std::endl;
    }
}

TreeManager::~TreeManager() {
    if (m_file) {
        m_file->Close();
        delete m_file;
    }
}

void TreeManager::fillDIS(const eicQuickSim::disKinematics& dis, double weight) {
    if (m_analysisType != "DIS") return;
    m_dis_Q2 = dis.Q2;
    m_dis_x  = dis.x;
    m_dis_y  = dis.y; 
    m_dis_W  = dis.W;
    m_weight = weight;
    m_tree->Fill();
}

void TreeManager::fillSIDIS(const eicQuickSim::sidisKinematics& sid, double weight) {
    if (m_analysisType != "SIDIS") return;
    m_sidis_Q2      = sid.Q2;
    m_sidis_x       = sid.x;
    m_sidis_y       = sid.y; 
    m_sidis_xF      = sid.xF;
    m_sidis_eta     = sid.eta;
    m_sidis_z       = sid.z;
    m_sidis_phi     = sid.phi;
    m_sidis_pt_lab  = sid.pT_lab;
    m_sidis_pt_com  = sid.pT_com;
    m_weight        = weight;
    m_tree->Fill();
}

void TreeManager::fillDISIDIS(const eicQuickSim::dihadronKinematics& dih, double weight) {
    if (m_analysisType != "DISIDIS") return;
    m_dihad_Q2             = dih.Q2;
    m_dihad_x              = dih.x;
    m_dihad_y              = dih.y; 
    m_dihad_z_pair         = dih.z_pair;
    m_dihad_phi_h          = dih.phi_h;
    m_dihad_phi_R_method0  = dih.phi_R_method0;
    m_dihad_phi_R_method1  = dih.phi_R_method1;
    m_dihad_pt_lab_pair    = dih.pT_lab_pair;
    m_dihad_pt_com_pair    = dih.pT_com_pair;
    m_dihad_xF_pair        = dih.xF_pair;
    m_dihad_com_th         = dih.com_th;
    m_dihad_Mh             = dih.Mh;
    m_weight               = weight;
    m_tree->Fill();
}

void TreeManager::saveTree() {
    if (!m_file || !m_tree) {
        std::cerr << "TreeManager: Cannot save, tree or file not initialized." << std::endl;
        return;
    }
    m_file->cd();
    m_tree->Write();
    std::cout << "TreeManager: TTree written to file successfully." << std::endl;
}
