#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <string>
#include <vector>
#include <functional>
#include "FileManager.h"
#include "Kinematics.h"
#include "Weights.h"
#include "BinningScheme.h"
#include "CombinedRowsProcessor.h"
#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderRootTree.h"

using namespace HepMC3;

namespace eicQuickSim {

class Analysis {
public:
    Analysis();
    ~Analysis();

    // Setters for each parameter.
    void setAnalysisType(const std::string& analysisType);
    void setEnergyConfig(const std::string& energyConfig);
    void setCSVSource(const std::string& csvSource); // number of files or CSV path.
    void setMaxEvents(int maxEvents);
    void setCollisionType(const std::string& collisionType);
    void setBinningSchemePath(const std::string& pathToBinScheme);
    void setOutputCSV(const std::string& outputCSV);
    
    // For DIS:
    void setDISValueFunction(std::function<std::vector<double>(const HepMC3::GenEvent&, const Kinematics&)> func);

    // For SIDIS: specify the particle id.
    void setSIDISPid(int pid);
    void setSIDISValueFunction(std::function<std::vector<double>(const sidisKinematics&)> func);

    // For DISIDIS: specify two particle ids.
    void setDISIDISPids(int pid1, int pid2);
    // And set a function to extract values from a dihadronKinematics struct.
    void setDihadValueFunction(std::function<std::vector<double>(const dihadronKinematics&)> func);

    // Run the analysis (process events) and then call end() to save the CSV.
    void run();
    void end();

private:
    // Input parameters.
    std::string m_analysisType;  // "DIS", "SIDIS", or "DISIDIS"
    std::string m_energyConfig;
    std::string m_csvSource;
    int m_maxEvents;
    std::string m_collisionType;
    std::string m_binningSchemePath;
    std::string m_outputCSV;

    // For SIDIS.
    int m_sidispid;
    std::function<std::vector<double>(const sidisKinematics&)> m_sidisValueFunction;

    // For DISIDIS.
    int m_dihad_pid1;
    int m_dihad_pid2;
    std::function<std::vector<double>(const dihadronKinematics&)> m_dihadValueFunction;

    // Data members.
    std::vector<CSVRow> m_combinedRows;
    Weights* m_q2Weights;
    BinningScheme* m_binScheme;

    // For DIS
    std::function<std::vector<double>(const HepMC3::GenEvent&, const Kinematics&)> m_disValueFunction;

    // Internal functions.
    bool checkInputs() const;
    void loadCSVRows();
};

} // namespace eicQuickSim

#endif // ANALYSIS_H
