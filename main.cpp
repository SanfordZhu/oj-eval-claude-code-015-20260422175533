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
    std::string dataDir = "data";
    static const int NUM_BUCKETS = 20;
    static const int COMPACT_THRESHOLD = 100; // Compact after 100 operations per bucket

    int getBucketId(const std::string& index) {
        unsigned int hash = 0;
        for (char c : index) {
            hash = hash * 31 + c;
        }
        return hash % NUM_BUCKETS;
    }

    std::string getBucketFilename(int bucketId) {
        return dataDir + "/bucket_" + std::to_string(bucketId) + ".txt";
    }

    std::string getOperationsFilename(int bucketId) {
        return dataDir + "/ops_" + std::to_string(bucketId) + ".txt";
    }

    void ensureDataDir() {
        if (!fs::exists(dataDir)) {
            fs::create_directory(dataDir);
        }
    }

    void compactBucket(int bucketId) {
        std::string bucketFile = getBucketFilename(bucketId);
        std::string opsFile = getOperationsFilename(bucketId);

        if (!fs::exists(opsFile)) return;

        // Read current state
        std::unordered_map<std::string, std::set<int>> data;

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
                    data[idx].insert(val);
                }
            }
            inFile.close();
        }

        // Apply operations
        std::ifstream opsIn(opsFile);
        std::string op;
        while (opsIn >> op) {
            if (op == "I") {
                std::string idx;
                int val;
                opsIn >> idx >> val;
                data[idx].insert(val);
            } else if (op == "D") {
                std::string idx;
                int val;
                opsIn >> idx >> val;
                data[idx].erase(val);
                if (data[idx].empty()) {
                    data.erase(idx);
                }
            }
        }
        opsIn.close();

        // Write compacted data
        std::ofstream outFile(bucketFile);
        for (const auto& [idx, values] : data) {
            outFile << idx;
            for (int v : values) {
                outFile << " " << v;
            }
            outFile << "\n";
        }
        outFile.close();

        // Clear operations file
        fs::remove(opsFile);
    }

public:
    FileStorage() {
        ensureDataDir();
    }

    void insert(const std::string& index, int value) {
        int bucketId = getBucketId(index);
        std::string opsFile = getOperationsFilename(bucketId);
        std::string bucketFile = getBucketFilename(bucketId);

        // Append operation to ops file
        std::ofstream opsOut(opsFile, std::ios::app);
        opsOut << "I " << index << " " << value << "\n";
        opsOut.close();

        // Check if we need to compact
        static std::unordered_map<int, int> opCount;
        opCount[bucketId]++;
        if (opCount[bucketId] >= COMPACT_THRESHOLD) {
            compactBucket(bucketId);
            opCount[bucketId] = 0;
        }
    }

    void deleteEntry(const std::string& index, int value) {
        int bucketId = getBucketId(index);
        std::string opsFile = getOperationsFilename(bucketId);
        std::string bucketFile = getBucketFilename(bucketId);

        // Append operation to ops file
        std::ofstream opsOut(opsFile, std::ios::app);
        opsOut << "D " << index << " " << value << "\n";
        opsOut.close();

        // Also check for compaction
        static std::unordered_map<int, int> opCount;
        opCount[bucketId]++;
        if (opCount[bucketId] >= COMPACT_THRESHOLD) {
            compactBucket(bucketId);
            opCount[bucketId] = 0;
        }
    }

    std::vector<int> find(const std::string& index) {
        int bucketId = getBucketId(index);
        std::string bucketFile = getBucketFilename(bucketId);
        std::string opsFile = getOperationsFilename(bucketId);

        // Build current state
        std::unordered_map<std::string, std::set<int>> currentData;

        // Read base data
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
                    currentData[idx].insert(val);
                }
            }
            inFile.close();
        }

        // Apply recent operations
        if (fs::exists(opsFile)) {
            std::ifstream opsIn(opsFile);
            std::string op;
            while (opsIn >> op) {
                if (op == "I") {
                    std::string idx;
                    int val;
                    opsIn >> idx >> val;
                    currentData[idx].insert(val);
                } else if (op == "D") {
                    std::string idx;
                    int val;
                    opsIn >> idx >> val;
                    currentData[idx].erase(val);
                    if (currentData[idx].empty()) {
                        currentData.erase(idx);
                    }
                }
            }
            opsIn.close();
        }

        // Return result
        auto it = currentData.find(index);
        if (it != currentData.end()) {
            return std::vector<int>(it->second.begin(), it->second.end());
        }
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