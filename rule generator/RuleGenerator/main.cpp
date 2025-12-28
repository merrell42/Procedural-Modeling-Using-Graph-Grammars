// main.cpp : Defines the entry point for the console application.
#include "pch.h"
#include "RuleGenerator.h"
#include <iostream>
#include <fstream>
#include <string>
#include <exception>

using namespace std;

int main(int argc, char* argv[])
{
    cout << "Hello World" << endl;
    
    if (argc < 2) {
        cout << "Usage: RuleGenerator.exe <input_json_file> [output_buffer_size]" << endl;
        cout << "   or: RuleGenerator.exe <json_string>" << endl;
        return 1;
    }

    string input;
    
    // Check if argument is a file path or JSON string.
    ifstream file(argv[1]);
    if (file.good()) {
        // Read from file.
        string line;
        while (getline(file, line)) {
            input += line;
        }
        file.close();
    } else {
        // Treat as JSON string.
        input = argv[1];
    }

    // Set output buffer size (default 1024).
    int maxLength = 1024;
    if (argc >= 3) {
        maxLength = atoi(argv[2]);
    }
    if (maxLength < 256) {
        maxLength = 256; // Minimum buffer size.
    }

    char* output = new char[maxLength];
    
    try {
        GenerateRules(input.c_str(), output, maxLength);
        cout << "Success: " << output << endl;
        delete[] output;
        return 0;
    } catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
        delete[] output;
        return 1;
    }
}

