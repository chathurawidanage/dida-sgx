#include <libcxx/string>
#include <libcxx/vector>
#include <libcxx/sstream>

#include "../Enclave.h"
#include "Enclave_t.h"

std::vector<char*> samFiles;

unsigned int cntgCount = 0;
unsigned int readCount = 0;

char newline_delim[] = "\n";

void ecall_init_merge(unsigned int maxCt, unsigned int maxRd) {
    cntgCount = maxCt;
    readCount = maxRd;
}

void fordMer(const int pNum, const std::string& alignerName) {
    std::string mapStr(alignerName);

    printf("Maximum target ID=%d\nTotal number of queries=%d\n", cntgCount, readCount);

    std::vector<std::vector<uint8_t> > ordList(readCount);

    unsigned int readId, bitFg;
    std::string readHead, headSQ;

    char* line = nullptr;
    // First pass, Reading
    int headEnd[pNum];
    for (int i = 0; i < pNum; ++i) {
        headEnd[i] = 0;
        line = strtok(samFiles[i], newline_delim);
        while (line != nullptr) {
            if (line[0] != '@') break;
            ++headEnd[i];
        }
        // inserting SAM info into ordList
        do {
            char** endptr;
            readId = std::strtol(line, endptr, 10);
            ordList[readId].push_back((uint8_t)(i + 1));
            line = strtok(NULL, newline_delim);
        } while (line != nullptr);
    }

    // Second pass, Writing
    std::string comFile = "";
    std::vector<char*> saved_ptrs(pNum);

    for (int i = 0; i < pNum; ++i) {
        //Discarding @
        for (int j = 0; j < headEnd[i]; ++j) {
            line = strtok_r(samFiles[i], newline_delim, &saved_ptrs[i]);
        }
    }

    char colChar;
    for (unsigned i = 0; i < readCount; ++i) {
        bool samVal = false;
        for (unsigned j = 0; j < ordList[i].size(); ++j) {
            line = strtok_r(samFiles[ordList[i][j] - 1], newline_delim, &saved_ptrs[i]);
            std::istringstream iss(line);
            iss>>readId>>colChar>>readHead>>bitFg;
            if (bitFg!=4) {
                samVal=true;
                size_t pos = line.find_first_of(":");
                comFile<<line.substr(pos+1, std::string::npos)<<"\n";
                //comFile << line << "\n";
            }
        }
        if (!samVal) {
            comFile.append(readHead).append("\t4\t*\t0\t0\t*\t*\t0\t0\t*\t*\n");
        }
    }

    printf("Final SAM : \n\n%s", comFile);
}

void ecall_load_sam(unsigned char* data, long char_len, int pid) {
    samFiles.push_back(reinterpret_cast<char*>(data));
    if (pid != samFiles.size()) {
        printf("Something wrong. Expected the sam file from process %d", pid);
    }
}

void ecall_load_sam_lreads(unsigned char* data, long char_len) {
    samFiles.push_back(reinterpret_cast<char*>(data));
}

void ecall_finalize_merge() {
    cntgCount = 0;
    readCount = 0;

    samFiles.clear();
}