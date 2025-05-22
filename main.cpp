#include <math.h>
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char *argv[])
{
    if(argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <mode> <input_file> [output_file]" << std::endl;
        std::cerr << "Modes: mtt, realizer, eos" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    std::string inputFilename = argv[2];
    std::string outputFilename = (argc > 3) ? argv[3] : "output.txt";
    
    std::cout << "Processing file: " << inputFilename << std::endl;
    std::cout << "Mode: " << mode << std::endl;
    
    // Check if input file exists
    std::ifstream inputFile(inputFilename);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Cannot open input file " << inputFilename << std::endl;
        return 1;
    }
    
    // Basic file processing based on mode
    if(mode == "mtt") {
        std::cout << "Processing MTT file..." << std::endl;
        
        // Read input file
        std::string line;
        std::ofstream outputFile(outputFilename);
        
        if (!outputFile.is_open()) {
            std::cerr << "Error: Cannot create output file " << outputFilename << std::endl;
            inputFile.close();
            return 1;
        }
        
        // Simple copy/processing
        outputFile << "# MTT Processing Output" << std::endl;
        while (std::getline(inputFile, line)) {
            outputFile << "MTT: " << line << std::endl;
        }
        
        outputFile.close();
        std::cout << "MTT processing complete. Output written to " << outputFilename << std::endl;
        
    } else if(mode == "realizer") {
        std::cout << "Processing Realizer file..." << std::endl;
        
        std::string line;
        std::ofstream outputFile(outputFilename);
        
        if (!outputFile.is_open()) {
            std::cerr << "Error: Cannot create output file " << outputFilename << std::endl;
            inputFile.close();
            return 1;
        }
        
        outputFile << "# Realizer Processing Output" << std::endl;
        while (std::getline(inputFile, line)) {
            outputFile << "REALIZER: " << line << std::endl;
        }
        
        outputFile.close();
        std::cout << "Realizer processing complete. Output written to " << outputFilename << std::endl;
        
    } else if(mode == "eos") {
        std::cout << "Processing EOS file..." << std::endl;
        
        std::string line;
        std::ofstream outputFile(outputFilename);
        
        if (!outputFile.is_open()) {
            std::cerr << "Error: Cannot create output file " << outputFilename << std::endl;
            inputFile.close();
            return 1;
        }
        
        outputFile << "# EOS Processing Output" << std::endl;
        while (std::getline(inputFile, line)) {
            outputFile << "EOS: " << line << std::endl;
        }
        
        outputFile.close();
        std::cout << "EOS processing complete. Output written to " << outputFilename << std::endl;
        
    } else {
        std::cerr << "Error: Invalid mode '" << mode << "'. Valid modes are: mtt, realizer, eos" << std::endl;
        inputFile.close();
        return 1;
    }
    
    inputFile.close();
    return 0;
}
