#include "Analysis.h"
#include "TreeManager.h"
#include <iostream>
#include <cctype>
#include <algorithm>
#include <yaml-cpp/yaml.h>

namespace eicQuickSim {

//////////////////////////////
// Helper functions
//////////////////////////////

std::string Analysis::toLower(const std::string& s) {
    std::string ret = s;
    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
    return ret;
}

double Analysis::getValueDIS(const disKinematics& dis, const std::string& branch) {
    std::string b = toLower(branch);
    if(b == "q2") return dis.Q2;
    else if(b == "x") return dis.x;
    else if(b == "w") return dis.W;
    else return 0.0;
}

double Analysis::getValueSIDIS(const sidisKinematics& sid, const std::string& branch) {
    std::string b = toLower(branch);
    if(b == "q2") return sid.Q2;
    else if(b == "x") return sid.x;
    else if(b == "xf") return sid.xF;
    else if(b == "eta") return sid.eta;
    else if(b == "z") return sid.z;
    else if(b == "phi") return sid.phi;
    else if(b == "pt_lab" || b=="ptlab") return sid.pT_lab;
    else if(b == "pt_com" || b=="ptcom") return sid.pT_com;
    else return 0.0;
}

double Analysis::getValueDihad(const dihadronKinematics& dih, const std::string& branch) {
    std::string b = toLower(branch);
    if(b == "q2") return dih.Q2;
    else if(b == "x") return dih.x;
    else if(b == "z_pair" || b=="zpair") return dih.z_pair;
    else if(b == "phi_h" || b=="phih") return dih.phi_h;
    else if(b == "phi_r_method0" || b=="phir0") return dih.phi_R_method0;
    else if(b == "phi_r_method1" || b=="phir1") return dih.phi_R_method1;
    else if(b == "pt_lab_pair" || b=="ptlabpair") return dih.pT_lab_pair;
    else if(b == "pt_com_pair" || b=="ptcompair") return dih.pT_com_pair;
    else if(b == "xf_pair" || b=="xfpair") return dih.xF_pair;
    else if(b == "com_th" || b=="comth") return dih.com_th;
    else if(b == "mh") return dih.Mh;
    else return 0.0;
}

//////////////////////////////
// Auto-generation of value functions
//////////////////////////////

// Assume that the BinningScheme class has a method that returns a vector<string>
// of branch_reco names (one for each dimension).
// For example: { "Q2", "X" } for DIS.
void Analysis::autoSetDISValueFunction() {
    std::vector<std::string> branches = m_binScheme->getReconstructedBranches();
    m_disValueFunction = [branches](const disKinematics& dis) -> std::vector<double> {
        std::vector<double> vals;
        for (const auto& b : branches) {
            vals.push_back(getValueDIS(dis, b));
        }
        return vals;
    };
}

void Analysis::autoSetSIDISValueFunction() {
    std::vector<std::string> branches = m_binScheme->getReconstructedBranches();
    m_sidisValueFunction = [branches](const sidisKinematics& sid) -> std::vector<double> {
        std::vector<double> vals;
        for (const auto& b : branches) {
            vals.push_back(getValueSIDIS(sid, b));
        }
        return vals;
    };
}

void Analysis::autoSetDihadValueFunction() {
    std::vector<std::string> branches = m_binScheme->getReconstructedBranches();
    m_dihadValueFunction = [branches](const dihadronKinematics& dih) -> std::vector<double> {
        std::vector<double> vals;
        for (const auto& b : branches) {
            vals.push_back(getValueDihad(dih, b));
        }
        return vals;
    };
}

//////////////////////////////
// Standard Analysis methods
//////////////////////////////

Analysis::Analysis() 
    : m_maxEvents(0), m_sidispid(0), m_dihad_pid1(0), m_dihad_pid2(0),
      m_treeManager(nullptr),
      m_q2Weights(nullptr), m_binScheme(nullptr)
{}

Analysis::~Analysis() {
    if(m_q2Weights) delete m_q2Weights;
    if(m_binScheme) delete m_binScheme;
    if(m_treeManager) delete m_treeManager;
}


void Analysis::initFromYaml(const std::string& yamlFile) {
    try {
        YAML::Node config = YAML::LoadFile(yamlFile);
        // Read keys from the YAML file and set member variables.
        m_analysisType    = config["analysis_type"].as<std::string>();
        m_energyConfig    = config["energy_config"].as<std::string>();
        m_csvSource       = config["csv_source"].as<std::string>();
        setCSVSource(m_csvSource);
        m_maxEvents       = config["max_events"].as<int>();
        m_collisionType   = config["collision_type"].as<std::string>();
        m_binningSchemePath = config["binning_scheme"].as<std::string>();
        m_outputCSV       = config["output_csv"].as<std::string>();
        enableTreeOutput(config["output_tree"].as<std::string>());
        
        // Set additional parameters if needed.
        if(m_analysisType == "SIDIS" && config["sidis_pid"]) {
            m_sidispid = config["sidis_pid"].as<int>();
        } else if(m_analysisType == "DISIDIS" && config["disidispid1"] && config["disidispid2"]) {
            m_dihad_pid1 = config["disidispid1"].as<int>();
            m_dihad_pid2 = config["disidispid2"].as<int>();
        }
        std::cout << "Loaded YAML configuration from " << yamlFile << std::endl;
    } catch(const std::exception &e) {
        std::cerr << "Error reading YAML file " << yamlFile << ": " << e.what() << std::endl;
    }
}

void Analysis::enableTreeOutput(const std::string& treeOutputFile) {
    m_treeManager = new TreeManager(treeOutputFile, m_analysisType);
}

void Analysis::setAnalysisType(const std::string& analysisType) {
    m_analysisType = analysisType;
}

void Analysis::setEnergyConfig(const std::string& energyConfig) {
    m_energyConfig = energyConfig;
}

void Analysis::setCSVSource(const std::string& csvSource) {
    m_csvSource = csvSource;
    std::string WeightsFilePath = csvSource;
    size_t pos = WeightsFilePath.rfind(".");
    if (pos != std::string::npos)
        WeightsFilePath = WeightsFilePath.substr(0, pos) + "_weights.csv";
    else
        WeightsFilePath += "_weights.csv";
    setCSVWeights(WeightsFilePath);
}

void Analysis::setCSVWeights(const std::string& csvWeights) {
    m_weightsPath = csvWeights;
}

void Analysis::setMaxEvents(int maxEvents) {
    m_maxEvents = maxEvents;
}

void Analysis::setCollisionType(const std::string& collisionType) {
    m_collisionType = collisionType;
}

void Analysis::setBinningSchemePath(const std::string& pathToBinScheme) {
    m_binningSchemePath = pathToBinScheme;
}

void Analysis::setOutputCSV(const std::string& outputCSV) {
    m_outputCSV = outputCSV;
}

void Analysis::setDISValueFunction(std::function<std::vector<double>(const disKinematics&)> func) {
    m_disValueFunction = func;
}

void Analysis::setSIDISValueFunction(std::function<std::vector<double>(const sidisKinematics&)> func) {
    m_sidisValueFunction = func;
}

void Analysis::setDihadValueFunction(std::function<std::vector<double>(const dihadronKinematics&)> func) {
    m_dihadValueFunction = func;
}

void Analysis::setSIDISPid(int pid) {
    m_sidispid = pid;
}

void Analysis::setDISIDISPids(int pid1, int pid2) {
    m_dihad_pid1 = pid1;
    m_dihad_pid2 = pid2;
}

bool Analysis::checkInputs() const {
    if(m_analysisType.empty() || m_energyConfig.empty() || m_csvSource.empty() ||
       m_collisionType.empty() || m_binningSchemePath.empty()) {
        std::cerr << "Missing required inputs for Analysis." << std::endl;
        return false;
    }
    if(m_analysisType != "DIS" && m_analysisType != "SIDIS" && m_analysisType != "DISIDIS") {
        std::cerr << "Unsupported analysis type: " << m_analysisType << std::endl;
        return false;
    }
    if(m_analysisType == "SIDIS") {
        if(m_sidispid == 0) {
            std::cerr << "For SIDIS, a valid particle id must be provided." << std::endl;
            return false;
        }
    }
    if(m_analysisType == "DISIDIS") {
        if(m_dihad_pid1 == 0 || m_dihad_pid2 == 0) {
            std::cerr << "For DISIDIS, two valid particle ids must be provided." << std::endl;
            return false;
        }
    }
    return true;
}

void Analysis::loadCSVRows() {
    bool isNumeric = true;
    for(char c : m_csvSource) {
        if(!std::isdigit(c)) { isNumeric = false; break; }
    }
    if(isNumeric) {
        int numFiles = std::stoi(m_csvSource);
        m_combinedRows = CombinedRowsProcessor::getCombinedRows(m_energyConfig, numFiles, m_maxEvents, m_collisionType);
    } else {
        FileManager fm(m_csvSource);
        m_combinedRows = fm.getAllCSVData(-1, -1);
    }
    std::cout << "Combined " << m_combinedRows.size() << " CSV rows." << std::endl;
}

void Analysis::run() {
    if(!checkInputs()) {
        std::cerr << "Analysis run aborted due to insufficient inputs." << std::endl;
        return;
    }
    loadCSVRows();
    m_q2Weights = new Weights(m_combinedRows, WeightInitMethod::PRECALCULATED, m_weightsPath);
    std::cout << "Q2=1.01 --> " << m_q2Weights->getWeight(1.01) << std::endl;
    std::cout << "Q2=10.01 --> " << m_q2Weights->getWeight(10.01) << std::endl;
    std::cout << "Q2=100.01 --> " << m_q2Weights->getWeight(100.01) << std::endl;
    
    if(m_binScheme) delete m_binScheme;
    m_binScheme = new BinningScheme(m_binningSchemePath);
    std::cout << "Loaded binning scheme for energy config: " << m_binScheme->getEnergyConfig() << "\n";

    // Automatically generate a value function if one has not been set.
    if(m_analysisType == "DIS" && !m_disValueFunction) {
        autoSetDISValueFunction();
    }
    if(m_analysisType == "SIDIS" && !m_sidisValueFunction) {
        autoSetSIDISValueFunction();
    }
    if(m_analysisType == "DISIDIS" && !m_dihadValueFunction) {
        autoSetDihadValueFunction();
    }
    
    // Process each CSV row (each representing a ROOT file).
    for(size_t i = 0; i < m_combinedRows.size(); ++i) {
        CSVRow row = m_combinedRows[i];
        std::string fullPath = row.filename;
        std::cout << "Processing file: " << fullPath << std::endl;
        
        ReaderRootTree root_input(fullPath);
        if(root_input.failed()) {
            std::cerr << "Failed to open file: " << fullPath << std::endl;
            continue;
        }
        
        int eventsParsed = 0;
        while(!root_input.failed() && eventsParsed < m_maxEvents) {
            HepMC3::GenEvent evt;
            root_input.read_event(evt);
            if(root_input.failed()) break;
            eventsParsed++;
            
            Kinematics kin;
            double eventWeight = 0.0;
            kin.computeDIS(evt); // Compute DIS first
            if(m_analysisType == "DIS") {
                disKinematics dis = kin.getDISKinematics();
                eventWeight = m_q2Weights->getWeight(dis.Q2);
                std::vector<double> values = m_disValueFunction(dis);
                m_binScheme->addEvent(values, eventWeight);
                if(m_treeManager) {
                    m_treeManager->fillDIS(dis, eventWeight);
                }
            }
            else if(m_analysisType == "SIDIS") {
                kin.computeSIDIS(evt, m_sidispid);
                disKinematics dis = kin.getDISKinematics();
                eventWeight = m_q2Weights->getWeight(dis.Q2);
                std::vector<sidisKinematics> sidis = kin.getSIDISKinematics();
                for(auto& sid : sidis) {
                    std::vector<double> values = m_sidisValueFunction(sid);
                    m_binScheme->addEvent(values, eventWeight);
                    if(m_treeManager) {
                        m_treeManager->fillSIDIS(sid, eventWeight);
                    }
                }
            }
            else if(m_analysisType == "DISIDIS") {
                kin.computeDISIDS(evt, m_dihad_pid1, m_dihad_pid2);
                disKinematics dis = kin.getDISKinematics();
                eventWeight = m_q2Weights->getWeight(dis.Q2);
                std::vector<dihadronKinematics> dihad = kin.getDISIDSKinematics();
                for(auto& dih : dihad) {
                    std::vector<double> values = m_dihadValueFunction(dih);
                    m_binScheme->addEvent(values, eventWeight);
                    if(m_treeManager) {
                        m_treeManager->fillDISIDIS(dih, eventWeight);
                    }
                }
            }
            else {
                std::cerr << "Unsupported analysis type: " << m_analysisType << std::endl;
                break;
            }
        }

        root_input.close();
    }
}

void Analysis::end() {
    if(m_outputCSV.empty()) {
        std::string binName = m_binScheme->getSchemeName();
        m_outputCSV = "artifacts/analysis_" + m_analysisType +
                      "_energy=" + m_energyConfig +
                      "_type=" + m_collisionType +
                      "_yamlName=" + binName +
                      "_maxEvents=" + std::to_string(m_maxEvents);
        bool isNumeric = true;
        for(char c : m_csvSource) {
            if(!std::isdigit(c)) { isNumeric = false; break; }
        }
        if(isNumeric) {
            m_outputCSV += "_numFiles=" + m_csvSource;
        }
        m_outputCSV += ".csv";
    }
    try {
        m_binScheme->saveCSV(m_outputCSV);
        std::cout << "Saved binned scaled event counts to " << m_outputCSV << std::endl;
    } catch(const std::exception &ex) {
        std::cerr << "Error saving CSV: " << ex.what() << std::endl;
    }
    if(m_treeManager) {
        try {
            m_treeManager->saveTree();
            std::cout << "Saved TTree to file via TreeManager." << std::endl;
        } catch(const std::exception &ex) {
            std::cerr << "Error saving TTree: " << ex.what() << std::endl;
        }
    }
}

} // namespace eicQuickSim
