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

void filInsert(DispatchCommand &command, std::vector<std::vector<bool> *> &myFilters, const unsigned pn, const std::string &bMer) {
    for (int i = 0; i < command.GetNHash(); ++i)
        (*myFilters[pn])[MurmurHash64A(bMer.c_str(), command.GetBmer(), i) % myFilters[pn]->size()] = true;
}

bool filContain(DispatchCommand &command, const std::vector<std::vector<bool> *> &myFilters, const unsigned pn, const std::string &bMer) {
    for (int i = 0; i < command.GetNHash(); ++i)
        if (!(*myFilters[pn])[MurmurHash64A(bMer.c_str(), command.GetBmer(), i) % myFilters[pn]->size()])
            return false;
    return true;
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

std::vector<std::vector<bool> *> loadFilter(DispatchCommand &command) {
#ifdef _OPENMP
    double start = omp_get_wtime();
#else
    clock_t sTime = clock();
#endif

#ifdef _OPENMP
    unsigned tNum = omp_get_max_threads() > opt::pnum ? opt::pnum : omp_get_max_threads();
    if (opt::threads < tNum && opt::threads > 0)
        tNum = opt::threads;
    std::cerr << "Number of threads=" << tNum << std::endl;
    omp_set_num_threads(tNum);
#endif

    int pIndex, chunk = 1;
    //begin create filters
    std::vector<std::vector<bool> *> myFilters(command.GetPartitions());

    std::cerr << "Loading filters ...\n";
#pragma omp parallel for shared(myFilters) private(pIndex) schedule(static, chunk)
    for (pIndex = 0; pIndex < command.GetPartitions(); ++pIndex) {
        std::stringstream sstm;
        sstm << command.GetIndexFolder() << "/";
        sstm << "mref-" << pIndex + 1 << ".fa";
        size_t filterSize = command.GetIBits() * getInfo((sstm.str()).c_str(), command.GetBmer());
        myFilters[pIndex] = new std::vector<bool>();
        myFilters[pIndex]->resize(filterSize);
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
                    filInsert(command, myFilters, pIndex, bMer);
                }
                line.clear();
            }
        }
        std::transform(line.begin(), line.end(), line.begin(), ::toupper);
        long uL = line.length();
        for (long j = 0; j < uL - command.GetBmer() + 1; ++j) {
            std::string bMer = line.substr(j, command.GetBmer());
            getCanon(command, bMer);
            filInsert(command, myFilters, pIndex, bMer);
        }

        uFile.close();
    }

    std::cerr << "Loading BF done!\n";
#ifdef _OPENMP
    std::cerr << "Loading in sec: " << omp_get_wtime() - start << "\n";
#else
    std::cerr << "Running time of loading in sec: " << (double)(clock() - sTime) / CLOCKS_PER_SEC << "\n";
#endif

    return myFilters;
}

/*void dispatchRead(const char *libName, const std::vector<std::vector<bool> *> &myFilters) {
    size_t buffSize = 4000000;
    std::ofstream rdFiles[opt::pnum];
    for (int i = 0; i < opt::pnum; ++i) {
        std::stringstream rstm;
        if (!opt::fq)
            rstm << "mreads-" << i + 1 << ".fa";
        else
            rstm << "mreads-" << i + 1 << ".fastq";
        rdFiles[i].open((rstm.str()).c_str());
    }
    std::ofstream msFile("lreads.sam");
    size_t fileNo = 0, readId = 0;
    std::string readHead, readSeq, readDir, readQual, rName;

    std::cerr << "reading from " << std::string(libName) << std::endl;

    std::ifstream libFile(libName);
    while (getline(libFile, rName)) {
        std::ifstream readFile[2];
        readFile[0].open(rName.c_str());  // taking one of the files

        std::cerr << "opening read file 1" << rName << std::endl;

        if (!opt::se) {
            getline(libFile, rName);
            readFile[1].open(rName.c_str());
            std::cerr << "opening read file 2" << rName << std::endl;
        }
        bool readValid = true;
        while (readValid) {
            readValid = false;
            // set up readBuff
            std::vector<faqRec> readBuffer;  // fixed-size to improve performance
            while (getline(readFile[fileNo], readHead)) {
                getline(readFile[fileNo], readSeq);
                std::transform(readSeq.begin(), readSeq.end(), readSeq.begin(), ::toupper);
                getline(readFile[fileNo], readDir);
                getline(readFile[fileNo], readQual);
                readHead[0] = ':';
                faqRec rRec;
                std::ostringstream hstm;
                if (!opt::fq)
                    hstm << ">" << readId << readHead;
                else
                    hstm << "@" << readId << readHead;
                rRec.readHead = hstm.str();
                rRec.readSeq = readSeq;
                rRec.readQual = readQual;
                readBuffer.push_back(rRec);
                if (!opt::se) fileNo = (fileNo + 1) % 2;
                ++readId;
                if (readBuffer.size() == buffSize) break;

                std::cout << "--------------------------------" << std::endl;

                std::cout << "readHead : " << readHead << std::endl;
                std::cout << "readSeq : " << readSeq << std::endl;
                std::cout << "readDir : " << readDir << std::endl;
                std::cout << "readQual : " << readQual << std::endl;

                std::cout << "--------------------------------" << std::endl;
            }
            if (readBuffer.size() == buffSize) readValid = true;

            //dispatch buffer
            int pIndex;
            std::vector<bool> dspRead(buffSize, false);
#pragma omp parallel for shared(readBuffer, rdFiles, dspRead) private(pIndex)
            for (pIndex = 0; pIndex < opt::pnum; ++pIndex) {
                for (size_t bIndex = 0; bIndex < readBuffer.size(); ++bIndex) {
                    faqRec bRead = readBuffer[bIndex];
                    size_t readLen = bRead.readSeq.length();
                    //size_t j=0;
                    for (size_t j = 0; j <= readLen - opt::bmer; j += opt::bmer_step) {
                        std::string bMer = bRead.readSeq.substr(j, opt::bmer);
                        getCanon(bMer);
                        if (filContain(myFilters, pIndex, bMer)) {
#pragma omp critical
                            dspRead[bIndex] = true;
                            if (!opt::fq)
                                rdFiles[pIndex] << bRead.readHead << "\n"
                                                << bRead.readSeq << "\n";
                            else
                                rdFiles[pIndex] << bRead.readHead << "\n"
                                                << bRead.readSeq << "\n+\n"
                                                << bRead.readQual << "\n";
                            break;
                        }
                    }
                }
            }  // end dispatch buffer
            for (size_t bIndex = 0; bIndex < readBuffer.size(); ++bIndex) {
                if (!dspRead[bIndex])
                    msFile << readBuffer[bIndex].readHead.substr(1, std::string::npos) << "\t4\t*\t0\t0\t*\t*\t0\t0\t*\t*\n";
            }
        }
        readFile[0].close();
        if (!opt::se)
            readFile[1].close();
    }
    libFile.close();
    msFile.close();
    for (int pIndex = 0; pIndex < opt::pnum; ++pIndex)
        rdFiles[pIndex].close();
    std::ofstream imdFile("maxinf", std::ios_base::app);
    imdFile << readId << "\n";
    imdFile.close();
}*/

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

std::vector<std::vector<bool> *> dida_build_bf(DispatchCommand &command) {
#ifdef _OPENMP
    double start = omp_get_wtime();
#else
    clock_t sTime = clock();
#endif

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

    std::string bf_backup_name = command.GetIndexFolder() + "/" + "nhash_" + std::to_string(command.GetNHash()) + "_ibits_" + std::to_string(command.GetIBits()) + "_bmersteps_" + std::to_string(command.GetBmerStep()) + "_bmer_" + std::to_string(command.GetBmer()) + "_part_" + std::to_string(command.GetPartitions()) + ".bf";

    spdlog::info("Bloomfilter location {}", bf_backup_name);

    std::vector<std::vector<bool> *> myFilters(command.GetPartitions());

    // check the file exists
    std::ifstream bf_file(bf_backup_name.c_str());
    if (bf_file.good()) {  // load from file
        std::cout << "Loading bloom filters from file" << std::endl;
        std::ifstream bf_in_file(bf_backup_name.c_str());
        std::cout << "Created if stream. P num : " << command.GetPartitions() << std::endl;
        for (int x = 0; x < command.GetPartitions(); x++) {
            std::vector<bool> *vec_ = new std::vector<bool>();
            binary_read(bf_in_file, vec_);
            std::cout << "Loaded a vector of size " << vec_->size() << std::endl;
            myFilters[x] = vec_;
        }
        bf_in_file.close();
        std::cout << "loaded bloom filters..." << std::endl;
    } else {
        bf_file.close();
        myFilters = loadFilter(command);

        // write to file
        std::cout << "backing up bloom filters..." << std::endl;
        std::ofstream bf_out_file(bf_backup_name.c_str());
        for (const std::vector<bool> *vec_ : myFilters) {
            binary_write(bf_out_file, vec_);
        }
        bf_out_file.close();
        std::cout << "bloom filter backed up..." << std::endl;
    }

    // move to sgx
    //dispatchRead(libName, myFilters);

#ifdef _OPENMP
    std::cerr << "Running time in sec: " << omp_get_wtime() - start << "\n";
#else
    std::cerr << "Running time in sec: " << (double)(clock() - sTime) / CLOCKS_PER_SEC << "\n";
#endif

    std::cout << "Returning vectors of size : " << myFilters.size() << std::endl;

    return myFilters;
}

// void dida_do_dsp(std::string libName, std::vector<std::vector<bool> *> bf) {
//     dispatchRead(libName.c_str(), bf);
// }