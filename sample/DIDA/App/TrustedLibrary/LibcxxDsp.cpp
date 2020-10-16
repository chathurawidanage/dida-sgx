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
#include "LibcxxDsp.h"

#include <getopt.h>
#include <stdio.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include "Enclave_u.h"
#include "dsp.h"

void dsp(int argc, char *argv[], sgx_enclave_id_t global_eid) {
    std::cout << "Loading bloom filters..." << std::endl;
    std::vector<std::vector<bool> *> bfs = dida_build_bf(argc, argv);
    std::cout << "Done loading bloom filters of length : " << bfs.size() << std::endl;

    for (int x = 0; x < bfs.size(); x++) {
        std::cout << "Encoding " << x << "th bfx" << std::endl;
        std::vector<bool> *bf = bfs[x];
        long bf_size = bf->size();

        std::cout << "BF size :  " << bf_size << std::endl;
        long char_arr_size = (bf_size / 8) + 1;
        unsigned char *bf_data = new unsigned char[char_arr_size];
        std::cout << "array size :  " << bf_size << std::endl;
        std::cout << "Encoding a bf of size " << bf->size() << std::endl;
        for (long i = 0; i < bf_size;) {
            unsigned char val = 0b00000000;
            for (int b = 7; b > -1 && i < bf_size; b--, i++) {
                val |= (bf->at(i) << b);
            }
            bf_data[(i / 8) - 1] = val;
        }
        printf("\n\n Sent\n\n");
        for (int i = 0; i < 40; i++) {
            printf("%s", bf->at(i) ? "1" : "0");
        }
        printf("\n");

        printf("\n\n Sent Chars\n\n");
        for (int i = 0; i < 5; i++) {
            printf("%d,", bf_data[i]);
        }
        printf("\n");

        delete bf;
        std::cout << "Sending a bf of size " << char_arr_size << " to the encalve..." << std::endl;
        sgx_status_t ret = ecall_load_bf(global_eid, bf_data, char_arr_size, bf_size);
        delete[] bf_data;
        if (ret != SGX_SUCCESS) {
            std::cerr << "Failed to add bloom filter to enclave" << std::endl;
        } else {
            printf("Back from enclave\n");
        }

        printf("\n");
    }

    ecall_print_bf_summary(global_eid);

    // PARSE ARGS
    const char shortopts[] = "s:l:b:p:j:d:h:i:r";

    enum { OPT_HELP = 1,
           OPT_VERSION };

    int bmer = 16;
    int bmer_step = -1;
    int nhash = 5;
    int se = 1;
    int fq = 0;
    int pnum = 12;
    unsigned threads = 0;
    int alen = 20;
    unsigned ibits = 8;

    const struct option longopts[] = {
        {"threads", required_argument, NULL, 'j'},
        {"partition", required_argument, NULL, 'p'},
        {"bmer", required_argument, NULL, 'b'},
        {"alen", required_argument, NULL, 'l'},
        {"step", required_argument, NULL, 's'},
        {"hash", required_argument, NULL, 'h'},
        {"bit", required_argument, NULL, 'i'},
        {"rebf", no_argument, NULL, 0},
        {"se", no_argument, &se, 1},
        {"fq", no_argument, &fq, 1},
        {"help", no_argument, NULL, OPT_HELP},
        {"version", no_argument, NULL, OPT_VERSION},
        {NULL, 0, NULL, 0}};

    bool die = false;
    std::string blPath;

    for (int c; (c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1;) {
        std::istringstream arg(optarg != NULL ? optarg : "");
        std::cout << "PARAM " << c << std::endl;
        switch (c) {
            case '?':
                die = true;
                break;
            case 'j':
                arg >> threads;
                break;
            case 'b':
                arg >> bmer;
                break;
            case 'p':
                arg >> pnum;
                break;
            case 'l':
                arg >> alen;
                break;
            case 's':
                arg >> bmer_step;
                break;
            case 'h':
                arg >> nhash;
                break;
            case 'i':
                arg >> ibits;
                break;
        }
    }

    if (bmer <= 0)
        bmer = 3 * alen / 4;

    if (bmer_step <= 0)
        bmer_step = alen - bmer + 1;

    std::cerr << "num-hash=" << nhash << "\n";
    std::cerr << "bit-item=" << ibits << "\n";
    std::cerr << "bmer-step=" << bmer_step << "\n";
    std::cerr << "bmer=" << bmer << "\n";
    std::cerr << "alen=" << alen << "\n";
    std::cerr << "pnum=" << pnum << "\n";
    // PARSE ARGS

    // now dispatch the reads
    std::cerr << "Dispatching file : " << argv[argc - 1] << std::endl;

    ecall_start_dispatch(global_eid, bmer,
                         bmer_step,
                         nhash,
                         se,
                         fq,
                         pnum);

    std::ifstream input_files_list(argv[argc - 1]);

    if (se) {
        // single ended

        // reading file
        std::string file_name;
        std::getline(input_files_list, file_name);

        std::ifstream input_file(file_name);

        std::string str((std::istreambuf_iterator<char>(input_file)),
                        std::istreambuf_iterator<char>());

        std::cerr << "Dispatch file length: " << str.size() << std::endl;
        sgx_status_t ret = ecall_load_data(global_eid, const_cast<char *>(str.c_str()), str.length());
        if (ret != SGX_SUCCESS) {
            std::cerr << "Failed to dispatch file : " << ret << std::endl;
        }
    } else {
        // paired ended
        std::string file_name_1;
        std::string file_name_2;
        std::getline(input_files_list, file_name_1);
        std::getline(input_files_list, file_name_2);

        std::ifstream input_file1(file_name_1);
        std::ifstream input_file2(file_name_2);

        std::string str1((std::istreambuf_iterator<char>(input_file1)),
                         std::istreambuf_iterator<char>());

        std::string str2((std::istreambuf_iterator<char>(input_file2)),
                         std::istreambuf_iterator<char>());

        std::cerr << "Dispatch file lengths: " << str1.size() << "," << str2.size() << std::endl;
        sgx_status_t ret = ecall_load_data2(global_eid,
                                            const_cast<char *>(str1.c_str()), str1.length(),
                                            const_cast<char *>(str2.c_str()), str2.length());
        if (ret != SGX_SUCCESS) {
            std::cerr << "Failed to dispatch file : " << ret << std::endl;
        }
    }
}
