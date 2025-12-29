// main.cpp : Defines the entry point for the console application.
#include "pch.h"
#include "RuleGenerator.h"
#include "../../cpp_version/json versioning/read_json_file.h"
#include <iostream>
#include <fstream>
#include <string>
#include <exception>

using namespace std;
using Json = nlohmann::json;

int main() {
    // Hardcoded path to square filled.json.
    const string jsonFilePath = "../primitives/square filled.json";
    
    // Set output buffer size (default 1024).
    int maxLength = 1024;
    if (maxLength < 256) {
        maxLength = 256; // Minimum buffer size.
    }

    char* output = new char[maxLength];
    
    try {
        // Read JSON file using readJsonFile to handle versioning properly.
        Json parsed = readJsonFile(jsonFilePath, false);
        
        // Convert to string for GenerateRules.
        string jsonString = parsed.dump();
        
        GenerateRules(jsonString.c_str(), output, maxLength);
        cout << "Success: " << output << endl;
        delete[] output;
        return 0;
    } catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
        delete[] output;
        return 1;
    }
}

