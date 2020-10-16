#include "LibcxxMrg.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Enclave_u.h"

void getInf(unsigned &maxCont, unsigned &maxRead) {
    std::ifstream infoFile("maxinf");
    if (infoFile.good()) {
        infoFile >> maxCont >> maxRead;
    } else {
        std::cerr << "Info file not exist!\n";
        exit(1);
    }
    if (maxCont == 0 || maxRead == 0) {
        std::cerr << "Error in info file!\n";
        exit(1);
    }
    infoFile.close();
    printf("Read maxinf %d, %d\n", maxCont, maxRead);
}

void mrg(int argc, char *argv[], sgx_enclave_id_t global_eid) {
    printf("Starting merging...\n");

    unsigned int maxCount = 0;
    unsigned int maxRead = 0;
    int pNum = std::atoi(argv[3]);

    std::string aligner = std::string(argv[5]);

    printf("Partitions : %d\nAligner : %s\n", pNum, aligner.c_str());

    getInf(maxCount, maxRead);
    printf("Our of reading maxinf %d, %d\n", maxCount, maxRead);

    ecall_init_merge(global_eid, maxCount, maxRead, pNum, (unsigned char *)aligner.c_str(), aligner.size());

    // read sam files
    for (int i = 0; i < pNum; ++i) {
        std::ifstream file("aln-" + std::to_string(i + 1) + ".sam");
        std::string str((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
        printf("loading sam file of size %ld \n", str.size());
        ecall_load_sam(global_eid, (char *)str.c_str(), str.size() + 1, i);
    }

    // now do the merging
    ecall_finalize_merge(global_eid);
}