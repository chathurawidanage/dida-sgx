#include "LibcxxMrg.h"

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

}