#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <atomic>
using namespace std;

const unsigned Num_of_Bins = 256;
int _numThreads;

void fileToMemoryTransfer(char* fileName, char** data, size_t& numOfBytes) {
    ifstream inFile(fileName, ios::in | ios::binary | ios::ate);
    if (!inFile)
    {
        cerr << "Cannot open " << fileName << endl;
        inFile.close();
        exit(1);
    }
    size_t size = inFile.tellg();
    char* buffer = new char[size];
    inFile.seekg(0, ios::beg);
    inFile.read(buffer, size);
    inFile.close();
    *data = buffer;
    numOfBytes = size;
}

atomic<unsigned long> h_CPU_naive[Num_of_Bins];
unsigned long h_CPU[Num_of_Bins];

int main(int argc, char* argv[]) {
    if (argc < 1 || argc > 2) {
        cerr << "Invalid number of arguments." << endl;
        exit(1);
    }
    _numThreads = thread::hardware_concurrency();
    size_t numOfBytes = 0;
    char* data;
    fileToMemoryTransfer(argv[1], &data, numOfBytes);

    unsigned blockSize = numOfBytes / _numThreads;
    vector<thread> workerThreads;

    for (int i = 0; i < _numThreads; i++) {
        unsigned start = i * blockSize;
        unsigned end = std::min(static_cast<size_t>((i + 1) * blockSize), numOfBytes);

        workerThreads.push_back(thread([=]() {
            for (unsigned i = start; i < end; ++i) {
                h_CPU_naive[static_cast<unsigned char>(data[i])]++;
            }
            }));
    }

    for (auto& t : workerThreads) {
        t.join();
    }

    cout << "Run with one global histogram" << endl;
    for (size_t i = 0; i < Num_of_Bins; i++) {
        cout << i << ": h(" << h_CPU_naive[i].load() << ")" << endl;
    }

    memset(h_CPU, 0, Num_of_Bins * sizeof(unsigned long));

    unsigned long* setOfLocalH = new unsigned long[_numThreads * Num_of_Bins];
    memset(setOfLocalH, 0, _numThreads * Num_of_Bins * sizeof(unsigned long));

    workerThreads.clear();

    for (int i = 0; i < _numThreads; i++) {
        unsigned start = i * blockSize;
        unsigned end = std::min(static_cast<size_t>((i + 1) * blockSize), numOfBytes);

        workerThreads.push_back(thread([=, &setOfLocalH]() {
            for (unsigned idx = start; idx < end; ++idx) {
                setOfLocalH[i * Num_of_Bins + static_cast<unsigned char>(data[idx])]++;
            }
            }));
    }

    for (auto& t : workerThreads) {
        t.join();
    }

    for (int i = 0; i < _numThreads; i++) {
        for (size_t j = 0; j < Num_of_Bins; j++) {
            h_CPU[j] += setOfLocalH[i * Num_of_Bins + j];
        }
    }

    cout << "Run with local histograms" << endl;
    for (size_t i = 0; i < Num_of_Bins; i++) {
        cout << i << ": h(" << h_CPU[i] << ")" << endl;
    }
     
    delete[] data; 
    delete[] setOfLocalH;

    return 0;
}
