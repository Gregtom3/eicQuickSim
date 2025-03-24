/**
 * A simple demonstration of how to use the FileManager class.
 *
 * This code expects "en_files.dat" to exist in the current directory,
 * containing lines of file paths like:
 *
 *   /volatile/eic/.../18x275/q2_100to1000/..._run098.ab.hepmc3.tree.root
 *
 * Then it tries to request a certain number of files for "18x275" and "100to1000".
 */

 #include "FileManager.h"
 #include <iostream>
 
 int main()
 {
     // Initialize the FileManager with your data file listing all the .root files
     FileManager fm("en_files.dat");
 
     // Let's request 5 files from the 18x275, q2_100to1000 group
     // If fewer than 5 exist, we just get them all
     auto myFiles = fm.getFiles("18x275", "100to1000", 5);
 
     // Print out what we got
     std::cout << "Retrieved " << myFiles.size() << " file(s):\n";
     for (auto &f : myFiles) {
         std::cout << "  " << f << "\n";
     }
 
     // Another example: ask for an invalid Q2 range
     auto badFiles = fm.getFiles("18x275", "999to9999", 2);
     if (badFiles.empty()) {
         std::cout << "[Test] No files returned for (18x275, 999to9999)\n";
     }
 
     // Negative or zero => get all for that group
     auto allFiles = fm.getFiles("18x275", "100to1000", -1);
     std::cout << "We asked for -1, so we got " << allFiles.size() << " file(s)\n";
 
     return 0;
 }
 