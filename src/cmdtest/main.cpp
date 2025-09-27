#include "LibGens.h"
#include "Compression.h"
#include "File.h"
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <future>
#include <queue>
#include <vector>
#include <condition_variable>
#include <map>

using namespace std;
using namespace LibGens;

string getFilename(const string& path) {
    size_t pos = path.find_last_of("/\\");
    return pos == string::npos ? path : path.substr(pos + 1);
}

void compressFile(const string& inputPath, CompressionType type) {
    string tempPath = inputPath + ".tmp"; // outputs temp file incase crash and corrupts main file
    
    File inputFile(inputPath, LIBGENS_FILE_READ_BINARY);
    
    if (!inputFile.valid()) {
        return;
    }

    uint32_t signature;
    inputFile.readInt32(&signature);
    if (Compression::check(signature)) {
        cout << "File is already Compressed! skipping.: " << inputPath << endl;
        inputFile.close();
        return;
    }
    inputFile.goToAddress(0);

    File outputFile(tempPath, LIBGENS_FILE_WRITE_BINARY);
    if (!outputFile.valid()) {
        inputFile.close();
        return;
    }

    cout << "> " << inputPath << endl;
    
    // CAB needs file name apparently
    string filename = getFilename(inputPath);
    Compression::compress(&inputFile, &outputFile, type, type == COMPRESSION_CAB ? &filename[0] : nullptr);
    
    inputFile.close();
    outputFile.close();

    if (remove(inputPath.c_str()) != 0) {
        remove(tempPath.c_str());
        return;
    }

    if (rename(tempPath.c_str(), inputPath.c_str()) != 0) {
        return;
    }
}

void decompressFile(const string& inputPath) {
    string tempPath = inputPath + ".tmp";
    
    File inputFile(inputPath, LIBGENS_FILE_READ_BINARY);
    File outputFile(tempPath, LIBGENS_FILE_WRITE_BINARY);

    if (!inputFile.valid() || !outputFile.valid()) {
        inputFile.close();
        outputFile.close();
        return;
    }

    uint32_t signature;
    inputFile.readInt32(&signature);
    inputFile.goToAddress(0);

    if (!Compression::check(signature)) {
        inputFile.close();
        outputFile.close();
        return;
    }

    CompressionType type;
    if (signature == COMPRESSION_CAB) type = COMPRESSION_CAB;
    else if (signature == COMPRESSION_X) type = COMPRESSION_X;
    else if (signature == COMPRESSION_SEGS) type = COMPRESSION_SEGS;
    else {
        inputFile.close();
        outputFile.close();
        return;
    }

    cout << "> " << inputPath << endl;
    
    Compression::decompress(&inputFile, &outputFile, type);
    
    inputFile.close();
    outputFile.close();

    if (remove(inputPath.c_str()) != 0) {
        remove(tempPath.c_str());
        return;
    }

    if (rename(tempPath.c_str(), inputPath.c_str()) != 0) {
        return;
    }
}

int main(int argc, char* argv[]) {
    cout << "HE1CompressionTool" << endl;

    if (argc < 2) {
        return 0;
    }

    string arg = argv[1];
    bool isCompressing = false;
    CompressionType compressType;

    if (arg == "-xcompress") {
        isCompressing = true;
        compressType = COMPRESSION_X;
    }
    else if (arg == "-genscompress") {
        isCompressing = true;
        compressType = COMPRESSION_CAB;
    }
    else if (arg != "-decompress") {
        return 0;
    }

    vector<future<void>> futures;
    size_t max_concurrent = thread::hardware_concurrency();
    size_t batch_size = max_concurrent * 2;

    for (int i = 2; i < argc; i++) {
        try {
            string current_file = argv[i];
            if (isCompressing) {
                futures.push_back(async(launch::async, [current_file, compressType]() {
                    compressFile(current_file, compressType);
                }));
            } else {
                futures.push_back(async(launch::async, [current_file]() {
                    decompressFile(current_file);
                }));
            }
            
            if (futures.size() >= batch_size) {
                for (auto& future : futures) {
                    future.get();
                }
                futures.clear();
            }
        }
        catch (...) { }
    }

    for (auto& future : futures) {
        future.get();
    }

    return 0;
}