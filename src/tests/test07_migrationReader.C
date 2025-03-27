#include "MigrationReader.h"
#include <iostream>
#include <vector>

int main() {
    try {
        // Create the reader using the YAML file.
        // Adjust the relative path if necessary.
        MigrationReader reader("src/tests/responseMatrices/test07_response_xQ2_5x41.yaml");

        // Print out a summary of the migration response.
        reader.printSummary();

        // Example: Get a response using flat indices.
        int totalBins = reader.getTotalBins();
        std::cout << "Total flattened bins: " << totalBins << std::endl;
        if (totalBins > 1) {
            double respFlat = reader.getResponse(0, 1);
            std::cout << "Response from flat indices 0 -> 1: " << respFlat << std::endl;
        }

        // Example: Get a response using multi-dimensional indices.
        // (For instance, in a 2D case, choose two indices per bin.)
        std::vector<int> trueIndices = {0, 1};  // example for 2 dimensions
        std::vector<int> recoIndices = {0, 2};
        double respMulti = reader.getResponse(trueIndices, recoIndices);
        std::cout << "Response from multi-indices (0,1)->(0,2): " << respMulti << std::endl;
    }
    catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
