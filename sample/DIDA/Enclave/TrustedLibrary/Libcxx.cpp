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

#include <libcxx/vector>
#include <libcxx/string>
#include "../Enclave.h"
#include "Enclave_t.h"
#include "sgx_tprotected_fs.h"

std::vector<std::vector<bool>> bloom_filters;

int bmer = 0;
int nhash = 0;

// MurmurHash2, 64-bit versions, by Austin Appleby
// https://sites.google.com/site/murmurhash/MurmurHash2_64.cpp?attredirects=0
uint64_t MurmurHash64A(const void *key, int len, unsigned int seed) {
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;

  uint64_t h = seed ^(len * m);

  const uint64_t *data = (const uint64_t *) key;
  const uint64_t *end = data + (len / 8);

  while (data != end) {
    uint64_t k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const unsigned char *data2 = (const unsigned char *) data;

  switch (len & 7) {
    case 7:h ^= uint64_t(data2[6]) << 48;
    case 6:h ^= uint64_t(data2[5]) << 40;
    case 5:h ^= uint64_t(data2[4]) << 32;
    case 4:h ^= uint64_t(data2[3]) << 24;
    case 3:h ^= uint64_t(data2[2]) << 16;
    case 2:h ^= uint64_t(data2[1]) << 8;
    case 1:h ^= uint64_t(data2[0]);
      h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

void filInsert(std::vector<std::vector<bool>> &myFilters, const unsigned pn, const std::string &bMer) {
  for (int i = 0; i < nhash; ++i)
    myFilters[pn][MurmurHash64A(bMer.c_str(), bmer, i) % myFilters[pn].size()] = true;
}

bool filContain(const std::vector<std::vector<bool>> &myFilters, const unsigned pn, const std::string &bMer) {
  for (int i = 0; i < nhash; ++i)
    if (!myFilters[pn][MurmurHash64A(bMer.c_str(), bmer, i) % myFilters[pn].size()])
      return false;
  return true;
}

void ecall_start_dispatch() {
  // do nothing for now
}

void ecall_finalize_dispatch() {
  //delete data;
}

void ecall_load_data(char *data_seq, int seq_len) {
  // do decryption

}

void ecall_load_bf(char *data, long len) {
  printf("adding a bloom filter of size : %d\n", len);
  std::vector<bool> bf;
  bf.reserve(len);
  for (int i = 0; i < len;) {
    char chr = data[i];
    for (int b = 0; b < 8 && i < len; b++, i++) {
      bf[i] = (chr & (1 << b)) != 0;
    }
  }
  bloom_filters.push_back(bf);
}

void ecall_print_bf_summary() {
  printf("No of bloom filters : %d\n", bloom_filters.size());
}