#pragma once
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#include <lemon/list_graph.h>

namespace rbconv {

    struct RBHeader {
        std::string title, key;
        int totcrd, ptrcrd, indcrd, valcrd, rhscrd;
        std::string mxtype;
        int nrow, ncol, nnzero, neltvl;
        std::string ptrfmt, indfmt, valfmt, rhsfmt;
    };

    class RBToLemonConverter {
    private:
        lemon::ListDigraph graph;
        lemon::ListDigraph::NodeMap<int>   nodeIds;
        lemon::ListDigraph::ArcMap<double> weights;

    public:
        RBToLemonConverter();

        bool readRutherfordBoeing(const std::string& filename);
        void saveToLemonFormat(const std::string& filename); 
        void saveToGraphML(const std::string& filename);    
        void printGraphStats();

    private:
        bool readHeader(std::ifstream& file, RBHeader& header);
        bool convertMatrixToGraph(std::ifstream& file, const RBHeader& header);
    };

} // namespace rbconv
