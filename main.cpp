#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

class FileStorage {
private:
    std::string dataDir = "data";

    std::string getFilename(const std::string& index) {
        // Create a safe filename from index
        std::string safeIndex;
        for (char c : index) {
            if (c == '/' || c == '\\' || c == '.' || c == ' ') {
                safeIndex += '_';
            } else {
                safeIndex += c;
            }
        }
        return dataDir + "/" + safeIndex + ".dat";
    }

    void ensureDataDir() {
        if (!fs::exists(dataDir)) {
            fs::create_directory(dataDir);
        }
    }

public:
    FileStorage() {
        ensureDataDir();
    }

    void insert(const std::string& index, int value) {
        std::string filename = getFilename(index);
        std::vector<int> values;

        // Read existing values
        if (fs::exists(filename)) {
            std::ifstream inFile(filename);
            int val;
            while (inFile >> val) {
                if (val != value) { // Avoid duplicates
                    values.push_back(val);
                }
            }
            inFile.close();
        }

        // Add new value
        values.push_back(value);

        // Sort values
        std::sort(values.begin(), values.end());

        // Write back to file
        std::ofstream outFile(filename);
        for (int val : values) {
            outFile << val << " ";
        }
        outFile.close();
    }

    void deleteEntry(const std::string& index, int value) {
        std::string filename = getFilename(index);

        if (!fs::exists(filename)) {
            return;
        }

        std::vector<int> values;

        // Read existing values
        std::ifstream inFile(filename);
        int val;
        while (inFile >> val) {
            if (val != value) {
                values.push_back(val);
            }
        }
        inFile.close();

        // Write back to file
        std::ofstream outFile(filename);
        for (int val : values) {
            outFile << val << " ";
        }
        outFile.close();

        // Delete file if empty
        if (values.empty()) {
            fs::remove(filename);
        }
    }

    std::vector<int> find(const std::string& index) {
        std::string filename = getFilename(index);
        std::vector<int> values;

        if (!fs::exists(filename)) {
            return values;
        }

        // Read values from file
        std::ifstream inFile(filename);
        int val;
        while (inFile >> val) {
            values.push_back(val);
        }
        inFile.close();

        return values;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    FileStorage storage;
    int n;
    std::cin >> n;
    std::cin.ignore();

    for (int i = 0; i < n; i++) {
        std::string line;
        std::getline(std::cin, line);
        std::istringstream iss(line);

        std::string command;
        iss >> command;

        if (command == "insert") {
            std::string index;
            int value;
            iss >> index >> value;
            storage.insert(index, value);
        } else if (command == "delete") {
            std::string index;
            int value;
            iss >> index >> value;
            storage.deleteEntry(index, value);
        } else if (command == "find") {
            std::string index;
            iss >> index;
            std::vector<int> values = storage.find(index);

            if (values.empty()) {
                std::cout << "null\n";
            } else {
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) std::cout << " ";
                    std::cout << values[j];
                }
                std::cout << "\n";
            }
        }
    }

    return 0;
}