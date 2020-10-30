#ifndef DIDA__DSP_H_
#define DIDA__DSP_H_

#include <vector>
#include <string>

std::vector<std::vector<bool>*> dida_build_bf(int argc, char **argv);
void dida_do_dsp(std::string libname, std::vector<std::vector<bool>*> bf);

#endif //DIDA__DSP_H_
