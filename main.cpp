#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <sstream>
#include <unordered_map>
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
        std::unordered_map<std::string, std::set<int>> allData;

        // Read existing data
        if (fs::exists(dataFile)) {
            std::ifstream inFile(dataFile);
            std::string line;
            while (std::getline(inFile, line)) {
                if (line.empty()) continue;
                std::istringstream iss(line);
                std::string idx;
                iss >> idx;
                int val;
                while (iss >> val) {
                    allData[idx].insert(val);
                }
            }
            inFile.close();
        }

        // Add new value
        allData[index].insert(value);

        // Write all data back
        std::ofstream outFile(dataFile);
        for (const auto& [idx, values] : allData) {
            outFile << idx;
            for (int val : values) {
                outFile << " " << val;
            }
            outFile << "\n";
        }
        outFile.close();
    }

    void deleteEntry(const std::string& index, int value) {
        if (!fs::exists(dataFile)) return;

        std::unordered_map<std::string, std::set<int>> allData;

        // Read existing data
        std::ifstream inFile(dataFile);
        std::string line;
        while (std::getline(inFile, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string idx;
            iss >> idx;
            int val;
            while (iss >> val) {
                allData[idx].insert(val);
            }
        }
        inFile.close();

        // Remove value
        auto it = allData.find(index);
        if (it != allData.end()) {
            it->second.erase(value);
            if (it->second.empty()) {
                allData.erase(it);
            }
        }

        // Write back
        std::ofstream outFile(dataFile);
        for (const auto& [idx, values] : allData) {
            outFile << idx;
            for (int val : values) {
                outFile << " " << val;
            }
            outFile << "\n";
        }
        outFile.close();
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