#include "LibcxxMrg.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "Enclave_u.h"

void getInf(unsigned &maxCont, unsigned &maxRead) {
    std::ifstream infoFile("maxinf");
    if (infoFile.good()){
        infoFile >> maxCont >> maxRead;
    }else {
        std::cerr << "Info file not exist!\n";
        exit(1);
    }
    if (maxCont==0 || maxRead==0) {
        std::cerr << "Error in info file!\n";
        exit(1);
    }
    infoFile.close();
}

void mrg(int argc, char *argv[], sgx_enclave_id_t global_eid) {
    printf("Starting merging...\n");

    unsigned int maxCount = 0;
    unsigned int maxRead = 0;
    int pNum = std::atoi(argv[3]);

    std::string aligner = std::string(argv[5]);

    printf("Partitions : %d\nAligner : %s\n", pNum, aligner.c_str());

    getInf(maxCount, maxRead);

    ecall_init_merge(global_eid, maxCount, maxRead, pNum, (unsigned char*)aligner.c_str(),aligner.size());

    // read sam files
    for (int i = 0; i < pNum; ++i) {
        std::ifstream t("aln-" + std::to_string(i + 1) + ".sam");
        std::stringstream buffer;
        buffer << t.rdbuf();
        std::string str = buffer.str();
        ecall_load_sam(global_eid, (unsigned char*)(str.c_str()),str.size(), i);
    }
}