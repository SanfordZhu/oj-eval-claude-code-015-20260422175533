#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <sstream>
#include <set>

namespace fs = std::filesystem;

class FileStorage {
private:
    std::string dataFile = "data/storage.txt";

    void ensureDataDir() {
        if (!fs::exists("data")) {
            fs::create_directory("data");
        }
    }

public:
    FileStorage() {
        ensureDataDir();
    }

    void insert(const std::string& index, int value) {
        std::set<int> values;
        bool found = false;

        // Read existing values for this index only
        if (fs::exists(dataFile)) {
            std::ifstream inFile(dataFile);
            std::string line;
            while (std::getline(inFile, line)) {
                if (line.empty()) continue;
                std::istringstream iss(line);
                std::string idx;
                iss >> idx;
                if (idx == index) {
                    found = true;
                    int val;
                    while (iss >> val) {
                        values.insert(val);
                    }
                    break;
                }
            }
            inFile.close();
        }

        // Add new value
        values.insert(value);

        // Create temporary file with updated data
        std::string tempFile = dataFile + ".tmp";
        std::ofstream outFile(tempFile);

        // Copy all data except the index we're updating
        if (fs::exists(dataFile)) {
            std::ifstream inFile(dataFile);
            std::string line;
            while (std::getline(inFile, line)) {
                if (line.empty()) continue;
                std::istringstream iss(line);
                std::string idx;
                iss >> idx;
                if (idx != index) {
                    outFile << line << "\n";
                }
            }
            inFile.close();
        }

        // Write the updated index
        outFile << index;
        for (int val : values) {
            outFile << " " << val;
        }
        outFile << "\n";
        outFile.close();

        // Replace original file
        fs::rename(tempFile, dataFile);
    }

    void deleteEntry(const std::string& index, int value) {
        if (!fs::exists(dataFile)) return;

        std::string tempFile = dataFile + ".tmp";
        std::ofstream outFile(tempFile);
        bool found = false;

        std::ifstream inFile(dataFile);
        std::string line;
        while (std::getline(inFile, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string idx;
            iss >> idx;
            if (idx == index) {
                found = true;
                std::set<int> values;
                int val;
                while (iss >> val) {
                    if (val != value) {
                        values.insert(val);
                    }
                }
                if (!values.empty()) {
                    outFile << index;
                    for (int v : values) {
                        outFile << " " << v;
                    }
                    outFile << "\n";
                }
            } else {
                outFile << line << "\n";
            }
        }
        inFile.close();
        outFile.close();

        fs::rename(tempFile, dataFile);
    }

    std::vector<int> find(const std::string& index) {
        if (!fs::exists(dataFile)) return {};

        std::ifstream inFile(dataFile);
        std::string line;
        while (std::getline(inFile, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string idx;
            iss >> idx;
            if (idx == index) {
                std::vector<int> values;
                int val;
                while (iss >> val) {
                    values.push_back(val);
                }
                inFile.close();
                return values;
            }
        }

        inFile.close();
        return {};
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