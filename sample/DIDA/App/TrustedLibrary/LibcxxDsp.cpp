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
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
using namespace std::chrono;

#include "Enclave_u.h"
#include "dsp.h"

void dsp(DispatchCommand &dispatch_command, sgx_enclave_id_t global_eid) {
    std::cout << "Loading bloom filters..." << std::endl;
    std::vector<bool> *bf = dida_build_bf(dispatch_command);
    std::cout << "Done loading bloom filters of length : " << bf->size() << std::endl;

    auto start = high_resolution_clock::now();

    {
        std::cout << "Encoding the bloomfilter..." << std::endl;
        long bf_size = bf->size();
        std::cout << "BF size :  " << bf_size << std::endl;
        long char_arr_size = (bf_size / 8) + 1;
        unsigned char *bf_data = new unsigned char[char_arr_size];
        //std::cout << "array size :  " << bf_size << std::endl;
        //std::cout << "Encoding a bf of size " << bf->size() << std::endl;
        for (long i = 0; i < bf_size;) {
            unsigned char val = 0b00000000;
            for (int b = 7; b > -1 && i < bf_size; b--, i++) {
                val |= (bf->at(i) << b);
            }
            bf_data[(i / 8) - 1] = val;
        }

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

    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(stop - start);

    std::cout << "Bloomfilter transfering took : " << duration.count() << "ms" << std::endl;

    ecall_print_bf_summary(global_eid);

    // now dispatch the reads

    ecall_start_dispatch(global_eid, dispatch_command.GetBmer(),
                         dispatch_command.GetBmerStep(),
                         dispatch_command.GetNHash(),
                         dispatch_command.GetSe(),
                         dispatch_command.GetFq(),
                         dispatch_command.GetPartitions(),
                         dispatch_command.GetSegment());

    if (dispatch_command.GetSe()) {
        // single ended

        // reading file

        std::ifstream input_file(dispatch_command.GetInput1());

        std::string str((std::istreambuf_iterator<char>(input_file)),
                        std::istreambuf_iterator<char>());

        std::cerr << "Dispatch file length: " << str.size() << std::endl;
        sgx_status_t ret = ecall_load_data(global_eid, const_cast<char *>(str.c_str()), str.length());
        std::cerr << "Dispatch done for segment " << dispatch_command.GetSegment() << std::endl;
        if (ret != SGX_SUCCESS) {
            std::cerr << "Failed to dispatch file : " << ret << std::endl;
        }
    } else {
        // paired ended
        std::ifstream input_file1(dispatch_command.GetInput1());
        std::ifstream input_file2(dispatch_command.GetInput2());

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
