#ifndef TREEMANAGER_H
#define TREEMANAGER_H

#include "Kinematics.h"
#include <string>

// ROOT headers
#include "TFile.h"
#include "TTree.h"

class TreeManager {
public:
    // Constructor: outputFile is the ROOT file to save the tree, analysisType indicates DIS, SIDIS, or DISIDIS.
    TreeManager(const std::string& outputFile, const std::string& analysisType);
    ~TreeManager();

    // Methods to fill TTree for different kinematics types.
    void fillDIS(const eicQuickSim::disKinematics& dis, double eventWeight);
    void fillSIDIS(const eicQuickSim::sidisKinematics& sid, double eventWeight);
    void fillDISIDIS(const eicQuickSim::dihadronKinematics& dih, double eventWeight);

    // Write the TTree to the output file and close.
    void saveTree();

private:
    // ROOT objects.
    TFile* m_file;
    TTree* m_tree;
    std::string m_analysisType;

    // Common event weight variable.
    double m_weight;

    // Variables for DIS analysis.
    double m_dis_Q2;
    double m_dis_x;
    double m_dis_y;   
    double m_dis_W;

    // Variables for SIDIS analysis.
    double m_sidis_Q2;
    double m_sidis_x;
    double m_sidis_y;   
    double m_sidis_xF;
    double m_sidis_eta;
    double m_sidis_z;
    double m_sidis_phi;
    double m_sidis_pt_lab;
    double m_sidis_pt_com;

    // Variables for DISIDIS analysis.
    double m_dihad_Q2;
    double m_dihad_x;
    double m_dihad_y;  
    double m_dihad_z_pair;
    double m_dihad_phi_h;
    double m_dihad_phi_R_method0;
    double m_dihad_phi_R_method1;
    double m_dihad_pt_lab_pair;
    double m_dihad_pt_com_pair;
    double m_dihad_xF_pair;
    double m_dihad_com_th;
    double m_dihad_Mh;
};

#endif // TREEMANAGER_H
