#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <sstream>
#include <unordered_map>
#include <cstring>

namespace fs = std::filesystem;

class FileStorage {
private:
    std::string dataDir = "data";
    std::string indexFile = dataDir + "/index.dat";
    static const int BUCKET_SIZE = 100; // Group 100 indices per file

    struct IndexEntry {
        char index[65];
        int fileId;
        int offset;
        int count;
    };

    int getFileId(const std::string& index) {
        // Simple hash to distribute indices across buckets
        unsigned int hash = 0;
        for (char c : index) {
            hash = hash * 31 + c;
        }
        return hash % BUCKET_SIZE;
    }

    std::string getBucketFilename(int fileId) {
        return dataDir + "/bucket_" + std::to_string(fileId) + ".dat";
    }

    void ensureDataDir() {
        if (!fs::exists(dataDir)) {
            fs::create_directory(dataDir);
        }
    }

    std::vector<int> readValuesFromOffset(const std::string& filename, int offset, int count) {
        std::vector<int> values;
        if (!fs::exists(filename) || count == 0) return values;

        std::ifstream inFile(filename, std::ios::binary);
        if (!inFile) return values;

        inFile.seekg(offset);
        for (int i = 0; i < count; i++) {
            int val;
            inFile.read(reinterpret_cast<char*>(&val), sizeof(int));
            values.push_back(val);
        }
        inFile.close();

        return values;
    }

    void writeValuesAtOffset(const std::string& filename, int offset, const std::vector<int>& values) {
        std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
        if (!file) {
            // Create file if it doesn't exist
            std::ofstream createFile(filename, std::ios::binary);
            createFile.close();
            file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        }

        file.seekp(offset);
        for (int val : values) {
            file.write(reinterpret_cast<const char*>(&val), sizeof(int));
        }
        file.close();
    }

public:
    FileStorage() {
        ensureDataDir();
    }

    void insert(const std::string& index, int value) {
        int fileId = getFileId(index);
        std::string bucketFile = getBucketFilename(fileId);

        // Read all data from bucket file
        std::unordered_map<std::string, std::vector<int>> bucketData;

        if (fs::exists(bucketFile)) {
            std::ifstream inFile(bucketFile, std::ios::binary);
            int numIndices;
            inFile.read(reinterpret_cast<char*>(&numIndices), sizeof(int));

            for (int i = 0; i < numIndices; i++) {
                IndexEntry entry;
                inFile.read(reinterpret_cast<char*>(&entry), sizeof(IndexEntry));

                std::vector<int> values = readValuesFromOffset(bucketFile, entry.offset, entry.count);
                bucketData[entry.index] = values;
            }
            inFile.close();
        }

        // Add value to the index
        auto& values = bucketData[index];
        auto it = std::lower_bound(values.begin(), values.end(), value);
        if (it == values.end() || *it != value) {
            values.insert(it, value);
        }

        // Write back to bucket file
        std::ofstream outFile(bucketFile, std::ios::binary);
        int numIndices = bucketData.size();
        outFile.write(reinterpret_cast<const char*>(&numIndices), sizeof(int));

        int currentOffset = sizeof(int) + numIndices * sizeof(IndexEntry);
        for (auto& [idx, vals] : bucketData) {
            IndexEntry entry;
            std::strncpy(entry.index, idx.c_str(), 64);
            entry.index[64] = '\0';
            entry.fileId = fileId;
            entry.offset = currentOffset;
            entry.count = vals.size();

            outFile.write(reinterpret_cast<const char*>(&entry), sizeof(IndexEntry));
            currentOffset += vals.size() * sizeof(int);
        }

        // Write values after all index entries
        for (auto& [idx, vals] : bucketData) {
            for (int val : vals) {
                outFile.write(reinterpret_cast<const char*>(&val), sizeof(int));
            }
        }

        outFile.close();
    }

    void deleteEntry(const std::string& index, int value) {
        int fileId = getFileId(index);
        std::string bucketFile = getBucketFilename(fileId);

        if (!fs::exists(bucketFile)) return;

        // Read all data from bucket file
        std::unordered_map<std::string, std::vector<int>> bucketData;

        std::ifstream inFile(bucketFile, std::ios::binary);
        int numIndices;
        inFile.read(reinterpret_cast<char*>(&numIndices), sizeof(int));

        for (int i = 0; i < numIndices; i++) {
            IndexEntry entry;
            inFile.read(reinterpret_cast<char*>(&entry), sizeof(IndexEntry));

            std::vector<int> values = readValuesFromOffset(bucketFile, entry.offset, entry.count);
            bucketData[entry.index] = values;
        }
        inFile.close();

        // Remove value from index
        auto it = bucketData.find(index);
        if (it != bucketData.end()) {
            auto& values = it->second;
            auto vit = std::lower_bound(values.begin(), values.end(), value);
            if (vit != values.end() && *vit == value) {
                values.erase(vit);
            }
            if (values.empty()) {
                bucketData.erase(it);
            }
        }

        // Write back to bucket file
        std::ofstream outFile(bucketFile, std::ios::binary);
        int newNumIndices = bucketData.size();
        outFile.write(reinterpret_cast<const char*>(&newNumIndices), sizeof(int));

        int currentOffset = sizeof(int) + newNumIndices * sizeof(IndexEntry);
        for (auto& [idx, vals] : bucketData) {
            IndexEntry entry;
            std::strncpy(entry.index, idx.c_str(), 64);
            entry.index[64] = '\0';
            entry.fileId = fileId;
            entry.offset = currentOffset;
            entry.count = vals.size();

            outFile.write(reinterpret_cast<const char*>(&entry), sizeof(IndexEntry));
            currentOffset += vals.size() * sizeof(int);
        }

        // Write values after all index entries
        for (auto& [idx, vals] : bucketData) {
            for (int val : vals) {
                outFile.write(reinterpret_cast<const char*>(&val), sizeof(int));
            }
        }

        outFile.close();

        if (bucketData.empty()) {
            fs::remove(bucketFile);
        }
    }

    std::vector<int> find(const std::string& index) {
        int fileId = getFileId(index);
        std::string bucketFile = getBucketFilename(fileId);

        if (!fs::exists(bucketFile)) return {};

        std::ifstream inFile(bucketFile, std::ios::binary);
        int numIndices;
        inFile.read(reinterpret_cast<char*>(&numIndices), sizeof(int));

        for (int i = 0; i < numIndices; i++) {
            IndexEntry entry;
            inFile.read(reinterpret_cast<char*>(&entry), sizeof(IndexEntry));

            if (std::strcmp(entry.index, index.c_str()) == 0) {
                inFile.close();
                return readValuesFromOffset(bucketFile, entry.offset, entry.count);
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