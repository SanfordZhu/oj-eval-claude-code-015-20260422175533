#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;

class FileStorage {
private:
    std::string dataDir = "data";
    static const int NUM_BUCKETS = 20; // Limited number of files

    int getBucketId(const std::string& index) {
        // Simple hash function
        unsigned int hash = 0;
        for (char c : index) {
            hash = hash * 31 + c;
        }
        return hash % NUM_BUCKETS;
    }

    std::string getBucketFilename(int bucketId) {
        return dataDir + "/bucket_" + std::to_string(bucketId) + ".txt";
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
        int bucketId = getBucketId(index);
        std::string bucketFile = getBucketFilename(bucketId);

        // Read all data from this bucket
        std::unordered_map<std::string, std::vector<int>> bucketData;

        if (fs::exists(bucketFile)) {
            std::ifstream inFile(bucketFile);
            std::string line;
            while (std::getline(inFile, line)) {
                if (line.empty()) continue;
                std::istringstream iss(line);
                std::string idx;
                iss >> idx;
                int val;
                while (iss >> val) {
                    bucketData[idx].push_back(val);
                }
            }
            inFile.close();
        }

        // Add value and keep sorted
        auto& values = bucketData[index];
        auto it = std::lower_bound(values.begin(), values.end(), value);
        if (it == values.end() || *it != value) {
            values.insert(it, value);
        }

        // Write back to bucket file
        std::ofstream outFile(bucketFile);
        for (const auto& [idx, vals] : bucketData) {
            outFile << idx;
            for (int v : vals) {
                outFile << " " << v;
            }
            outFile << "\n";
        }
        outFile.close();
    }

    void deleteEntry(const std::string& index, int value) {
        int bucketId = getBucketId(index);
        std::string bucketFile = getBucketFilename(bucketId);

        if (!fs::exists(bucketFile)) return;

        // Read all data from this bucket
        std::unordered_map<std::string, std::vector<int>> bucketData;

        std::ifstream inFile(bucketFile);
        std::string line;
        while (std::getline(inFile, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string idx;
            iss >> idx;
            int val;
            while (iss >> val) {
                if (idx == index && val == value) {
                    // Skip this value
                    continue;
                }
                bucketData[idx].push_back(val);
            }
        }
        inFile.close();

        // Write back to bucket file
        std::ofstream outFile(bucketFile);
        for (const auto& [idx, vals] : bucketData) {
            outFile << idx;
            for (int v : vals) {
                outFile << " " << v;
            }
            outFile << "\n";
        }
        outFile.close();

        // Delete file if empty
        if (bucketData.empty()) {
            fs::remove(bucketFile);
        }
    }

    std::vector<int> find(const std::string& index) {
        int bucketId = getBucketId(index);
        std::string bucketFile = getBucketFilename(bucketId);

        if (!fs::exists(bucketFile)) return {};

        // Read bucket file and find the index
        std::ifstream inFile(bucketFile);
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