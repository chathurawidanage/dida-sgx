#include "dsp.h"

#include <getopt.h>
#include <stdint.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "Uncompress.h"
#include "config.h"
#include "spdlog/spdlog.h"
#ifdef _OPENMP
#include <omp.h>
#endif

#define PROGRAM "dsp"

static const char b2p[256] = {
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //0
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //1
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //2
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //3
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'T', 'N', 'G', 'N', 'N', 'N', 'C',  //4   'A' 'C' 'G'
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'A', 'N', 'N', 'N',  //5   'T'
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'T', 'N', 'G', 'N', 'N', 'N', 'C',  //6   'a' 'c' 'g'
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'A', 'N', 'N', 'N',  //7   't'
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //8
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //9
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //10
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //11
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //12
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //13
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //14
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',  //15
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N'};

struct faqRec {
    std::string readHead;
    std::string readSeq;
    std::string readQual;
};

size_t getInfo(const char *aName, unsigned k) {
    std::string line;
    std::ifstream faFile(aName);

    std::cerr << "Getting info of " << aName << "\n";

    getline(faFile, line);
    if (line[0] != '>') {
        std::cerr << "Target file is not in correct format!\n";
        exit(EXIT_FAILURE);
        return 0;
    }
    size_t totItm = 0, uLen = 0;
    while (getline(faFile, line)) {
        if (line[0] != '>')
            uLen += line.length();
        else {
            if (uLen >= k)
                totItm += uLen - k + 1;
            uLen = 0;
        }
    }
    if (uLen >= k)
        totItm += uLen - k + 1;

    std::cerr << "Got info of " << aName << "\n";
    std::cerr << "|totLen|=" << totItm << "\n";
    faFile.close();
    return totItm;
}

// MurmurHash2, 64-bit versions, by Austin Appleby
// https://sites.google.com/site/murmurhash/MurmurHash2_64.cpp?attredirects=0
uint64_t MurmurHash64A(const void *key, int len, unsigned int seed) {
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t *data = (const uint64_t *)key;
    const uint64_t *end = data + (len / 8);

    while (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char *data2 = (const unsigned char *)data;

    switch (len & 7) {
        case 7:
            h ^= uint64_t(data2[6]) << 48;
        case 6:
            h ^= uint64_t(data2[5]) << 40;
        case 5:
            h ^= uint64_t(data2[4]) << 32;
        case 4:
            h ^= uint64_t(data2[3]) << 24;
        case 3:
            h ^= uint64_t(data2[2]) << 16;
        case 2:
            h ^= uint64_t(data2[1]) << 8;
        case 1:
            h ^= uint64_t(data2[0]);
            h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

void filInsert(DispatchCommand &command, std::vector<bool> *myFilter, const std::string &bMer) {
    for (int i = 0; i < command.GetNHash(); ++i)
        (*myFilter)[MurmurHash64A(bMer.c_str(), command.GetBmer(), i) % myFilter->size()] = true;
}

void getCanon(DispatchCommand &command, std::string &bMer) {
    int p = 0, hLen = (command.GetBmer() - 1) / 2;
    while (bMer[p] == b2p[(unsigned char)bMer[command.GetBmer() - 1 - p]]) {
        ++p;
        if (p >= hLen) break;
    }
    if (bMer[p] > b2p[(unsigned char)bMer[command.GetBmer() - 1 - p]]) {
        for (int lIndex = p, rIndex = command.GetBmer() - 1 - p; lIndex <= rIndex; ++lIndex, --rIndex) {
            char tmp = b2p[(unsigned char)bMer[rIndex]];
            bMer[rIndex] = b2p[(unsigned char)bMer[lIndex]];
            bMer[lIndex] = tmp;
        }
    }
}

std::vector<bool> *loadFilter(DispatchCommand &command) {
    clock_t sTime = clock();

    int pIndex, chunk = 1;
    //begin create filters
    std::vector<bool> *myFilter;

    std::cerr << "Loading filters ...\n";
    //handel segmenting
    std::stringstream sstm;
    sstm << command.GetIndexFolder() << "/";
    sstm << "mref-" << command.GetSegment() + 1 << ".fa";
    size_t filterSize = command.GetIBits() * getInfo((sstm.str()).c_str(), command.GetBmer());
    myFilter = new std::vector<bool>();
    myFilter->resize(filterSize);
    std::ifstream uFile((sstm.str()).c_str());

    std::string pline, line;
    getline(uFile, pline);
    while (getline(uFile, pline)) {
        if (pline[0] != '>')
            line += pline;
        else {
            std::transform(line.begin(), line.end(), line.begin(), ::toupper);
            long uL = line.length();
            for (long j = 0; j < uL - command.GetBmer() + 1; ++j) {
                std::string bMer = line.substr(j, command.GetBmer());
                getCanon(command, bMer);
                filInsert(command, myFilter, bMer);
            }
            line.clear();
        }
    }
    std::transform(line.begin(), line.end(), line.begin(), ::toupper);
    long uL = line.length();
    for (long j = 0; j < uL - command.GetBmer() + 1; ++j) {
        std::string bMer = line.substr(j, command.GetBmer());
        getCanon(command, bMer);
        filInsert(command, myFilter, bMer);
    }

    uFile.close();

    std::cerr << "Loading BF done!\n";
    std::cerr << "Running time of loading in sec: " << (double)(clock() - sTime) / CLOCKS_PER_SEC << "\n";
    return myFilter;
}

void binary_write(std::ofstream &fout, const std::vector<bool> *x) {
    std::vector<bool>::size_type n = x->size();
    fout.write((const char *)&n, sizeof(std::vector<bool>::size_type));
    for (std::vector<bool>::size_type i = 0; i < n;) {
        unsigned char aggr = 0;
        for (unsigned char mask = 1; mask > 0 && i < n; ++i, mask <<= 1)
            if (x->at(i))
                aggr |= mask;
        fout.write((const char *)&aggr, sizeof(unsigned char));
    }
}

void binary_read(std::ifstream &fin, std::vector<bool> *x) {
    std::vector<bool>::size_type n;
    fin.read((char *)&n, sizeof(std::vector<bool>::size_type));
    x->resize(n);
    for (std::vector<bool>::size_type i = 0; i < n;) {
        unsigned char aggr;
        fin.read((char *)&aggr, sizeof(unsigned char));
        for (unsigned char mask = 1; mask > 0 && i < n; ++i, mask <<= 1)
            x->at(i) = aggr & mask;
    }
}

std::vector<bool> *dida_build_bf(DispatchCommand &command) {
    clock_t sTime = clock();

    bool die = false;
    std::string blPath;

    if (command.GetAln() <= 1) {
        std::cerr << PROGRAM ": alignment length must at least 2.\n";
        die = true;
    }

    if (command.GetBmer() <= 0) {
        command.SetBmer(2 * command.GetAln() / 4);
    }

    if (command.GetBmerStep() <= 0) {
        command.SetBmerStep(command.GetAln() - command.GetBmer() + 1);
    }

    std::cerr << "num-hash=" << command.GetNHash() << "\n";
    std::cerr << "bmer-step=" << command.GetBmerStep() << "\n";
    std::cerr << "bmer=" << command.GetBmer() << "\n";
    std::cerr << "alen=" << command.GetAln() << "\n";

    std::string bf_backup_name = command.GetIndexFolder() + "/" + "nhash_" + std::to_string(command.GetNHash()) + "_ibits_" + std::to_string(command.GetIBits()) + "_bmersteps_" + std::to_string(command.GetBmerStep()) + "_bmer_" + std::to_string(command.GetBmer()) + "_part_" + std::to_string(command.GetPartitions()) + "_segm_" + std::to_string(command.GetSegment()) + ".bf";

    spdlog::info("Bloomfilter location {}", bf_backup_name);

    std::vector<bool> *myFilter;

    // check the file exists
    std::ifstream bf_file(bf_backup_name.c_str());
    if (bf_file.good()) {  // load from file
        std::cout << "Loading bloom filters from file" << std::endl;
        std::ifstream bf_in_file(bf_backup_name.c_str());
        std::cout << "Created if stream. P num : " << command.GetPartitions() << std::endl;
        // if segmentation enabled load only one bf
        myFilter = new std::vector<bool>();
        binary_read(bf_in_file, myFilter);
        std::cout << "Loaded a vector of size in segment mode" << myFilter->size() << std::endl;
        bf_in_file.close();
        std::cout << "loaded bloom filters..." << std::endl;
    } else {
        bf_file.close();
        myFilter = loadFilter(command);

        // write to file
        std::cout << "backing up bloom filters..." << std::endl;
        std::ofstream bf_out_file(bf_backup_name.c_str());
        binary_write(bf_out_file, myFilter);
        bf_out_file.close();
        std::cout << "bloom filter backed up..." << std::endl;
    }

    // move to sgx
    //dispatchRead(libName, myFilters);

    std::cout << "Returning vectors of size : " << myFilter->size() << std::endl;

    return myFilter;
}

// void dida_do_dsp(std::string libName, std::vector<std::vector<bool> *> bf) {
//     dispatchRead(libName.c_str(), bf);
// }