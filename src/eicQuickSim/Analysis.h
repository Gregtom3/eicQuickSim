#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <string>
#include <vector>
#include <functional>
#include "FileManager.h"
#include "Kinematics.h"
#include "TreeManager.h"
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
    void initFromYaml(const std::string& yamlFile);
    // Setters for each parameter.
    void setAnalysisType(const std::string& analysisType);
    void setEnergyConfig(const std::string& energyConfig);
    void setCSVSource(const std::string& csvSource); // number of files or CSV path.
    void setCSVWeights(const std::string& csvWeights); // CSV containing Q2 weights
    void setMaxEvents(int maxEvents);
    void setCollisionType(const std::string& collisionType);
    void setBinningSchemePath(const std::string& pathToBinScheme);
    void setOutputCSV(const std::string& outputCSV);
    void setSIDISPid(int pid);
    void setDISIDISPids(int pid1, int pid2);

    // For DIS: set a custom value function that extracts a vector<double> from disKinematics.
    void setDISValueFunction(std::function<std::vector<double>(const disKinematics&)> func);
    // For SIDIS: set a custom value function that extracts a vector<double> from sidisKinematics.
    void setSIDISValueFunction(std::function<std::vector<double>(const sidisKinematics&)> func);
    // For DISIDIS: set a custom value function that extracts a vector<double> from dihadronKinematics.
    void setDihadValueFunction(std::function<std::vector<double>(const dihadronKinematics&)> func);

    void enableTreeOutput(const std::string& treeOutputFile);
    
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
    std::string m_weightsPath;

    // For SIDIS.
    int m_sidispid;

    // For DISIDIS.
    int m_dihad_pid1;
    int m_dihad_pid2;

    // User-defined value functions.
    std::function<std::vector<double>(const disKinematics&)> m_disValueFunction;
    std::function<std::vector<double>(const sidisKinematics&)> m_sidisValueFunction;
    std::function<std::vector<double>(const dihadronKinematics&)> m_dihadValueFunction;

    // Data members.
    std::vector<CSVRow> m_combinedRows;
    Weights* m_q2Weights;
    BinningScheme* m_binScheme;
    TreeManager* m_treeManager;

    // Internal functions.
    bool checkInputs() const;
    void loadCSVRows();

    // --- Helper functions for auto-generating value functions ---
    // These functions map a branch name (from the YAML) to a value from the struct.
    static std::string toLower(const std::string& s);
    static double getValueDIS(const disKinematics& dis, const std::string& branch);
    static double getValueSIDIS(const sidisKinematics& sid, const std::string& branch);
    static double getValueDihad(const dihadronKinematics& dih, const std::string& branch);

    // Auto-generate a DIS value function based on the bin scheme.
    void autoSetDISValueFunction();
    void autoSetSIDISValueFunction();
    void autoSetDihadValueFunction();
};

} // namespace eicQuickSim

#endif // ANALYSIS_H
