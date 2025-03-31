#include "Analysis.h"
#include <iostream>
#include <cctype>

namespace eicQuickSim {

Analysis::Analysis() 
    : m_maxEvents(0), m_sidispid(0), m_dihad_pid1(0), m_dihad_pid2(0),
      m_q2Weights(nullptr), m_binScheme(nullptr)
{}

Analysis::~Analysis() {
    if(m_q2Weights) delete m_q2Weights;
    if(m_binScheme) delete m_binScheme;
}

void Analysis::setAnalysisType(const std::string& analysisType) {
    m_analysisType = analysisType;
}

void Analysis::setEnergyConfig(const std::string& energyConfig) {
    m_energyConfig = energyConfig;
}

void Analysis::setCSVSource(const std::string& csvSource) {
    m_csvSource = csvSource;
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

void Analysis::setDISValueFunction(std::function<std::vector<double>(const HepMC3::GenEvent&, const Kinematics&)> func) {
    m_disValueFunction = func;
}

void Analysis::setSIDISPid(int pid) {
    m_sidispid = pid;
}

void Analysis::setDISIDISPids(int pid1, int pid2) {
    m_dihad_pid1 = pid1;
    m_dihad_pid2 = pid2;
}

void Analysis::setSIDISValueFunction(std::function<std::vector<double>(const sidisKinematics&)> func) {
    m_sidisValueFunction = func;
}

void Analysis::setDihadValueFunction(std::function<std::vector<double>(const dihadronKinematics&)> func) {
    m_dihadValueFunction = func;
}

bool Analysis::checkInputs() const {
    if(m_analysisType.empty() || m_energyConfig.empty() || m_csvSource.empty() ||
       m_collisionType.empty() || m_binningSchemePath.empty() || m_maxEvents <= 0) {
        std::cerr << "Missing required inputs for Analysis." << std::endl;
        return false;
    }
    if(m_analysisType != "DIS" && m_analysisType != "SIDIS" && m_analysisType != "DISIDIS") {
        std::cerr << "Unsupported analysis type: " << m_analysisType << std::endl;
        return false;
    }
    if(m_analysisType == "SIDIS" && m_sidispid == 0) {
        std::cerr << "For SIDIS, a valid particle id must be provided." << std::endl;
        return false;
    }
    if(m_analysisType == "DISIDIS") {
        if(m_dihad_pid1 == 0 || m_dihad_pid2 == 0) {
            std::cerr << "For DISIDIS, two valid particle ids must be provided." << std::endl;
            return false;
        }
        if(!m_dihadValueFunction) {
            std::cerr << "For DISIDIS, a dihadron value function must be provided." << std::endl;
            return false;
        }
    }
    if((m_analysisType == "DIS") && !m_disValueFunction) {
        std::cerr << "For DIS, a DIS value function must be defined." << std::endl;
        return false;
    }
    if(m_analysisType == "SIDIS" && !m_sidisValueFunction) {
        std::cerr << "For SIDIS, a SIDIS value function must be provided." << std::endl;
        return false;
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
    m_q2Weights = new Weights(m_combinedRows);
    if(m_collisionType == "ep")
        m_q2Weights->loadExperimentalLuminosity("src/eicQuickSim/ep_lumi.csv");
    else
        m_q2Weights->loadExperimentalLuminosity("src/eicQuickSim/en_lumi.csv");
    
    if(m_binScheme) delete m_binScheme;
    m_binScheme = new BinningScheme(m_binningSchemePath);
    std::cout << "Loaded binning scheme for energy config: " << m_binScheme->getEnergyConfig() << "\n";
    
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
            kin.computeDIS(evt);
            if(m_analysisType == "DIS") {
                disKinematics dis = kin.getDISKinematics();
                eventWeight = m_q2Weights->getWeight(dis.Q2);
                std::vector<double> values = m_disValueFunction(evt, kin);
                m_binScheme->addEvent(values, eventWeight);
            }
            else if(m_analysisType == "SIDIS") {
                kin.computeSIDIS(evt, m_sidispid);
                disKinematics dis = kin.getDISKinematics();
                eventWeight = m_q2Weights->getWeight(dis.Q2);
                // Loop over all SIDIS final-state hadrons.
                std::vector<sidisKinematics> sidis = kin.getSIDISKinematics();
                for(auto& sid : sidis) {
                    std::vector<double> values = m_sidisValueFunction(sid);
                    m_binScheme->addEvent(values, eventWeight);
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
}

} // namespace eicQuickSim
