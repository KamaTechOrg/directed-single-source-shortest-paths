#include "rb_to_lemon.hpp"
#include <string>
#include <cstdio>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::printf("Usage: %s <rutherford_boeing_file>\n", argv[0]);
        return 1;
    }
    std::string inputFile = argv[1];

    rbconv::RBToLemonConverter conv;
    if (!conv.readRutherfordBoeing(inputFile)) return 1;

    conv.printGraphStats();

    std::string base = inputFile;
    size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);

    conv.saveToLemonFormat(base + "_graph.lgf");
    conv.saveToGraphML(base + "_graph.graphml");
    return 0;
}
