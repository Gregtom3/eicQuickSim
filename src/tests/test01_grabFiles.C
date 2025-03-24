/**
 * Test code demonstrating how we read a CSV-based FileManager
 * and retrieve file names by (e, h, Q2 range).
 */

 #include "FileManager.h"
 #include <iostream>
 #include <vector>
 
 int main() {
     // Suppose we have "summary.csv" in the same directory
     FileManager fm("src/eicQuickSim/en_files.csv");
 
     // Let's try to get 5 files for e=10, h=100, Q2=10..100
     auto files = fm.getFiles(10, 100, 10, 100, 5);
     std::cout << "We got " << files.size() << " files:\n";
     for (const auto &f : files) {
         std::cout << "  " << f << "\n";
     }
 
     // Another example: ask for e=18, h=275, Q2=100..1000
     auto files2 = fm.getFiles(18, 275, 100, 1000, 2);
     std::cout << "We got " << files2.size() << " file(s) for e=18,h=275,Q2=100..1000:\n";
     for (const auto &f : files2) {
         std::cout << "  " << f << "\n";
     }
 
     return 0;
 }
 