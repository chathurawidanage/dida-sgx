/*
 * Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <libcxx/string>
#include <libcxx/vector>

#include "../Enclave.h"
#include "Enclave_t.h"
#include "sgx_tprotected_fs.h"

struct faqRec {
    std::string readHead;
    std::string readSeq;
    std::string readQual;
};

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

std::vector<std::vector<bool> *> bloom_filters;

int bmer = 0;
int bmer_step = 0;
int nhash = 0;
int se = 0;
int fq = 0;
int pnum = 0;
int segment = -1;

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

void filInsert(std::vector<std::vector<bool> *> &myFilters, const unsigned pn, const std::string &bMer) {
    for (int i = 0; i < nhash; ++i)
        (*myFilters[pn])[MurmurHash64A(bMer.c_str(), bmer, i) % myFilters[pn]->size()] = true;
}

bool filContain(const std::vector<std::vector<bool> *> &myFilters, const unsigned pn, const std::string &bMer) {
    for (int i = 0; i < nhash; ++i) {
        if (!myFilters[pn]->at(MurmurHash64A(bMer.c_str(), bmer, i) % myFilters[pn]->size()))
            return false;
    }
    return true;
}

void getCanon(std::string &bMer) {
    int p = 0, hLen = (bmer - 1) / 2;
    while (bMer[p] == b2p[(unsigned char)bMer[bmer - 1 - p]]) {
        ++p;
        if (p >= hLen) break;
    }
    if (bMer[p] > b2p[(unsigned char)bMer[bmer - 1 - p]]) {
        for (int lIndex = p, rIndex = bmer - 1 - p; lIndex <= rIndex; ++lIndex, --rIndex) {
            char tmp = b2p[(unsigned char)bMer[rIndex]];
            bMer[rIndex] = b2p[(unsigned char)bMer[lIndex]];
            bMer[lIndex] = tmp;
        }
    }
}

void dispatchRead(char *sequence1, int seq1_len, char *sequence2, int seq2_len) {
    size_t buffSize = 4000000;

    std::vector<std::string *> rdFiles;

    for (int i = 0; i < pnum; i++) {
        rdFiles.push_back(new std::string(""));
    }

    std::vector<char *> sequences;
    sequences.push_back(sequence1);

    if (!se) {
        sequences.push_back(sequence2);
    }

    std::string msFile;
    std::string imdFile;

    size_t fileNo = 0, readId = 0;
    std::string readHead, readSeq, readDir, readQual, rName;

    char delim[] = "\n";

    std::vector<char *> saved_ptrs(sequences.size());
    std::vector<bool> saved_ptrs_init(sequences.size());
    for (int i = 0; i < sequences.size(); i++) {
        saved_ptrs[i] = sequences[i];
        saved_ptrs_init[i] = false;
    }

    bool readValid = true;
    printf("Starting outer loop...\n");
    while (readValid) {
        readValid = false;
        // set up readBuff
        std::vector<faqRec *> readBuffer;  // fixed-size to improve performance
        //printf("Processing file %d\n", fileNo);
        char *line = strtok_r(sequences[fileNo], delim, &saved_ptrs[fileNo]);
        saved_ptrs_init[fileNo] = true;
        while (line != nullptr) {
            readHead = line;
            //printf("Head of file no %d : %s\n", fileNo, readHead);
            line = strtok_r(NULL, delim, &saved_ptrs[fileNo]);
            if (line != nullptr) {
                readSeq = line;
            } else {
                printf("FATAL : Null sequence\n");
                continue;
            }
            std::transform(readSeq.begin(), readSeq.end(), readSeq.begin(), ::toupper);
            line = strtok_r(NULL, delim, &saved_ptrs[fileNo]);
            if (line != nullptr) {
                readDir = line;
            }

            line = strtok_r(NULL, delim, &saved_ptrs[fileNo]);
            if (line != nullptr) {
                readQual = line;
            }

            readHead[0] = ':';
            faqRec *rRec = new faqRec;

            std::string hstm;
            if (!fq) {
                hstm.append(">").append(std::to_string(readId)).append(readHead);
            } else {
                hstm.append("@").append(std::to_string(readId)).append(readHead);
            };
            rRec->readHead = hstm;
            rRec->readSeq = readSeq;
            rRec->readQual = readQual;
            readBuffer.push_back(rRec);

            //printf("readHead from file %d : %s\n", fileNo, hstm.c_str());
            //printf("readSeq from file %d : %s\n", fileNo,readSeq.c_str());

            if (!se) fileNo = (fileNo + 1) % 2;
            ++readId;
            if (readBuffer.size() == buffSize) break;

            if (saved_ptrs_init[fileNo]) {
                line = strtok_r(NULL, delim, &saved_ptrs[fileNo]);
            } else {
                line = strtok_r(sequences[fileNo], delim, &saved_ptrs[fileNo]);
                saved_ptrs_init[fileNo] = true;
            }
            // line = strtok(NULL, delim);
            // if (line == nullptr && !se && fileNo == 1) {  // move to next file
            //     printf("File no at the end : %d\n", fileNo);
            //     line = strtok(sequences[fileNo], delim);
            // }
        }

        printf("Done filling buffers\n");
        if (readBuffer.size() == buffSize) readValid = true;

        //dispatch buffer
        int pIndex;
        std::vector<bool> dspRead(buffSize, false);
        for (pIndex = 0; pIndex < pnum; ++pIndex) {
            //printf("Processing partition %d having %d buffers\n", pIndex, readBuffer.size());
            for (size_t bIndex = 0; bIndex < readBuffer.size(); ++bIndex) {
                //printf("Processing read buff %d\n", bIndex);
                faqRec *bRead = readBuffer[bIndex];
                size_t readLen = bRead->readSeq.length();
                //printf("Seq len  %d\n", readLen);
                //size_t j=0;
                for (size_t j = 0; j <= readLen - bmer; j += bmer_step) {
                    //printf("Processing bmer %d\n", j);
                    std::string bMer = bRead->readSeq.substr(j, bmer);
                    //printf("Get canon...\n");
                    getCanon(bMer);
                    //printf("Checking bloomfilter v2... %s, pNum : %d \n", bMer, pIndex);
                    // todo optimize here
                    if (filContain(bloom_filters, pIndex, bMer)) {
                        //printf("Checked bloomfilter...\n");
                        dspRead[bIndex] = true;
                        if (!fq)
                            rdFiles[pIndex]->append(bRead->readHead).append("\n").append(bRead->readSeq).append("\n");
                        else
                            rdFiles[pIndex]->append(bRead->readHead).append("\n").append(bRead->readSeq).append("\n+\n").append(bRead->readQual).append("\n");
                        break;
                    }
                }
            }
        }  // end dispatch buffer
        for (size_t bIndex = 0; bIndex < readBuffer.size(); ++bIndex) {
            if (!dspRead[bIndex])
                msFile.append(readBuffer[bIndex]->readHead.substr(1, std::string::npos))
                    .append("\t4\t*\t0\t0\t*\t*\t0\t0\t*\t*\n");
        }

        printf("End of outer while loop\n");
    }

    imdFile.append(std::to_string(readId)).append("\n");

    //printf("\n\n");
    //printf("msFile content : \n\n");
    //printf(msFile.c_str());
    //printf("\n\n");

    //printf("imFile content : \n\n");
    std::string max_inf = "maxinf";
    ocall_print_file(imdFile.c_str(), max_inf.c_str(), 1);
    printf("maxinf : %s", imdFile.c_str());
    //printf("\n\n");

    printf("printing %d rdFiles %d...\n", rdFiles.size(), pnum);
    for (int i = 0; i < rdFiles.size(); i++) {
        // printf("rdFile %d content : \n\n", i);
        // printf(rdFiles[i]->c_str());
        // printf("\n\n");

        std::string file_name = "mread-" + std::to_string(i + 1) + ".fa";
        printf("Doing ocall to write to file : %s, Size : %d\n", file_name, rdFiles[i]->size());
        print_file(file_name.c_str(), 0, rdFiles[i]->c_str());
    }
}

void dispatchReadSingleSegment(char *sequence1, int seq1_len, char *sequence2, int seq2_len) {
    size_t buffSize = 4000000;

    std::string *rdFile = new std::string("");
    // std::vector<std::string *> rdFiles;

    // for (int i = 0; i < pnum; i++) {
    //     rdFiles.push_back(new std::string(""));
    // }

    std::vector<char *> sequences;
    sequences.push_back(sequence1);

    if (!se) {
        sequences.push_back(sequence2);
    }

    std::string msFile;
    std::string imdFile;

    size_t fileNo = 0, readId = 0;
    std::string readHead, readSeq, readDir, readQual, rName;

    char delim[] = "\n";

    std::vector<char *> saved_ptrs(sequences.size());
    std::vector<bool> saved_ptrs_init(sequences.size());
    for (int i = 0; i < sequences.size(); i++) {
        saved_ptrs[i] = sequences[i];
        saved_ptrs_init[i] = false;
    }

    bool readValid = true;
    printf("Starting outer loop...\n");
    while (readValid) {
        readValid = false;
        // set up readBuff
        std::vector<faqRec *> readBuffer;  // fixed-size to improve performance
        //printf("Processing file %d\n", fileNo);
        char *line = strtok_r(sequences[fileNo], delim, &saved_ptrs[fileNo]);
        saved_ptrs_init[fileNo] = true;
        while (line != nullptr) {
            readHead = line;
            //printf("Head of file no %d : %s\n", fileNo, readHead);
            line = strtok_r(NULL, delim, &saved_ptrs[fileNo]);
            if (line != nullptr) {
                readSeq = line;
            } else {
                printf("FATAL : Null sequence\n");
                continue;
            }
            std::transform(readSeq.begin(), readSeq.end(), readSeq.begin(), ::toupper);
            line = strtok_r(NULL, delim, &saved_ptrs[fileNo]);
            if (line != nullptr) {
                readDir = line;
            }

            line = strtok_r(NULL, delim, &saved_ptrs[fileNo]);
            if (line != nullptr) {
                readQual = line;
            }

            readHead[0] = ':';
            faqRec *rRec = new faqRec;

            std::string hstm;
            if (!fq) {
                hstm.append(">").append(std::to_string(readId)).append(readHead);
            } else {
                hstm.append("@").append(std::to_string(readId)).append(readHead);
            };
            rRec->readHead = hstm;
            rRec->readSeq = readSeq;
            rRec->readQual = readQual;
            readBuffer.push_back(rRec);

            //printf("readHead from file %d : %s\n", fileNo, hstm.c_str());
            //printf("readSeq from file %d : %s\n", fileNo,readSeq.c_str());

            if (!se) fileNo = (fileNo + 1) % 2;
            ++readId;
            if (readBuffer.size() == buffSize) break;

            if (saved_ptrs_init[fileNo]) {
                line = strtok_r(NULL, delim, &saved_ptrs[fileNo]);
            } else {
                line = strtok_r(sequences[fileNo], delim, &saved_ptrs[fileNo]);
                saved_ptrs_init[fileNo] = true;
            }
            // line = strtok(NULL, delim);
            // if (line == nullptr && !se && fileNo == 1) {  // move to next file
            //     printf("File no at the end : %d\n", fileNo);
            //     line = strtok(sequences[fileNo], delim);
            // }
        }

        printf("Done filling buffers\n");
        if (readBuffer.size() == buffSize) readValid = true;

        //dispatch buffer
        int pIndex = 0;  //only one partition is handled in this mode
        std::vector<bool> dspRead(buffSize, false);
        // for (pIndex = 0; pIndex < pnum; ++pIndex) {

        //printf("Processing partition %d having %d buffers\n", pIndex, readBuffer.size());
        for (size_t bIndex = 0; bIndex < readBuffer.size(); ++bIndex) {
            //printf("Processing read buff %d\n", bIndex);
            faqRec *bRead = readBuffer[bIndex];
            size_t readLen = bRead->readSeq.length();
            //printf("Seq len  %d\n", readLen);
            //size_t j=0;
            for (size_t j = 0; j <= readLen - bmer; j += bmer_step) {
                //printf("Processing bmer %d\n", j);
                std::string bMer = bRead->readSeq.substr(j, bmer);
                //printf("Get canon...\n");
                getCanon(bMer);
                //printf("Checking bloomfilter v2... %s, pNum : %d \n", bMer, pIndex);
                // todo optimize here
                if (filContain(bloom_filters, pIndex, bMer)) {
                    //printf("Checked bloomfilter...\n");
                    dspRead[bIndex] = true;
                    if (!fq)
                        rdFile->append(bRead->readHead).append("\n").append(bRead->readSeq).append("\n");
                    else
                        rdFile->append(bRead->readHead).append("\n").append(bRead->readSeq).append("\n+\n").append(bRead->readQual).append("\n");
                    break;
                }
            }
        }
        // }  // end dispatch buffer
        for (size_t bIndex = 0; bIndex < readBuffer.size(); ++bIndex) {
            if (!dspRead[bIndex])
                msFile.append(readBuffer[bIndex]->readHead.substr(1, std::string::npos))
                    .append("\t4\t*\t0\t0\t*\t*\t0\t0\t*\t*\n");

            // clear memory
            delete readBuffer[bIndex];
        }

        printf("End of outer while loop\n");
    }

    imdFile.append(std::to_string(readId)).append("\n");

    //printf("\n\n");
    //printf("msFile content : \n\n");
    //printf(msFile.c_str());
    //printf("\n\n");

    //printf("imFile content : \n\n");
    std::string max_inf = "maxinf-" + segment;
    ocall_print_file(imdFile.c_str(), max_inf.c_str(), 1);
    printf("maxinf : %s", imdFile.c_str());
    //printf("\n\n");

    std::string file_name = "mread-" + std::to_string(segment + 1) + ".fa";
    printf("Doing ocall to write to file : %s, Size : %d\n", file_name, rdFile->size());
    print_file(file_name.c_str(), 0, rdFile->c_str());
}

void ecall_start_dispatch(int para_bmer,
                          int para_bmer_step,
                          int para_nhash,
                          int para_se,
                          int para_fq,
                          int para_pnum,
                          int sgmnt) {
    bmer = para_bmer;
    bmer_step = para_bmer_step;
    nhash = para_nhash;
    se = para_se;
    fq = para_fq;
    pnum = para_pnum;
    segment = sgmnt;

    if (segment != -1) {
        printf("handeling segment %d\n", segment);
    }

    printf("Initialized dispatch with pnum %d\n", pnum);
}

void ecall_finalize_dispatch() {
    //delete data;
}

void ecall_load_data(char *data_seq, int seq_len) {
    // do decryption
    printf("Dispatching read of length : %d, pnum: %d\n", seq_len, pnum);
    if (segment != -1) {
        dispatchReadSingleSegment(data_seq, seq_len, nullptr, 0);
    } else {
        dispatchRead(data_seq, seq_len, nullptr, 0);
    }
}

void ecall_load_data2(char *data1, int len1, char *data2, int len2) {
    // do decryption of both seq here
    printf("Dispatching paired read of length : %d and %d, pnum: %d\n", len1, len2, pnum);
    if (segment != -1) {
        dispatchReadSingleSegment(data1, len1, data2, len2);
    } else {
        dispatchRead(data1, len1, data2, len2);
    }
}

void ecall_load_bf(unsigned char *data, long len, long bf_len) {
    //printf("adding a bloom filter of size : %d\n", len);
    // printf("\n\n Received Chars\n\n");
    // for (int i = 0; i < 5; i++) {
    //     printf("%d,", data[i]);
    // }
    // printf("\n");

    std::vector<bool> *bf = new std::vector<bool>();
    bf->reserve(bf_len);
    for (long i = 0; i < len; i++) {
        unsigned char chr = data[i];
        for (int b = 7; b > -1 && bf->size() < bf_len; b--) {
            bf->push_back((chr & (0b00000001 << b)) != 0);
        }
    }

    // printf("\n\n Received\n\n");
    // for (int i = 0; i < 40; i++) {
    //     printf("%s", bf->at(i) ? "1" : "0");
    // }
    //printf("\nAdding to the bloom filters...\n");
    bloom_filters.push_back(bf);
    //printf("\nAdded to the bloom filters...\n");

    //printf("Deleting received char\n");
}

void ecall_print_bf_summary() {
    printf("No of bloom filters : %d\n", bloom_filters.size());
    for (int i = 0; i < bloom_filters.size(); i++) {
        printf("\tSize of bf %ld\n", bloom_filters[i]->size());
    }
}