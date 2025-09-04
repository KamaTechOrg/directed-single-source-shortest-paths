#include "rb_to_lemon.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <vector>

using rbconv::RBHeader;
using rbconv::RBToLemonConverter;

RBToLemonConverter::RBToLemonConverter()
    : nodeIds(graph), weights(graph) {}

namespace {
    // Threshold for treating weights as ~0 (instead of a magic number literal)
    constexpr double kMinWeightThreshold = 1e-12;
}

bool RBToLemonConverter::readRutherfordBoeing(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    RBHeader header;
    if (!readHeader(file, header)) {
        std::cerr << "Error: Failed to read header" << std::endl;
        return false;
    }

    std::cout << "Reading matrix: " << header.title << std::endl;
    std::cout << "Dimensions: " << header.nrow << " x " << header.ncol << std::endl;
    std::cout << "Non-zeros: " << header.nnzero << std::endl;

    return convertMatrixToGraph(file, header);
}

bool RBToLemonConverter::readHeader(std::ifstream& file, RBHeader& h) {
    std::string line;

    // Line 1: Title and Key
    if (!std::getline(file, line)) return false;
    if (line.size() >= 72) {
        h.title = line.substr(0, 72);
        if (line.size() >= 80) h.key = line.substr(72, 8);
    }

    // Line 2: counts
    if (!std::getline(file, line)) return false;
    {
        std::stringstream ss(line);
        ss >> h.totcrd >> h.ptrcrd >> h.indcrd >> h.valcrd >> h.rhscrd;
    }

    // Line 3: type + dims
    if (!std::getline(file, line)) return false;
    {
        std::stringstream ss(line);
        ss >> h.mxtype >> h.nrow >> h.ncol >> h.nnzero >> h.neltvl;
    }

    // Line 4: formats
    if (!std::getline(file, line)) return false;
    {
        std::stringstream ss(line);
        ss >> h.ptrfmt >> h.indfmt >> h.valfmt >> h.rhsfmt;
    }

    return true;
}

bool RBToLemonConverter::convertMatrixToGraph(std::ifstream& file, const RBHeader& h) {
    std::vector<lemon::ListDigraph::Node> nodes;
    int maxDim = std::max(h.nrow, h.ncol);
    nodes.reserve(maxDim);
    for (int i = 0; i < maxDim; ++i) {
        lemon::ListDigraph::Node n = graph.addNode();
        nodeIds[n] = i;
        nodes.push_back(n);
    }

    // Column pointers
    std::vector<int> colPtr(h.ncol + 1);
    for (int i = 0; i <= h.ncol; ++i) {
        file >> colPtr[i];
        --colPtr[i]; // 1-based -> 0-based
    }

    // Row indices
    std::vector<int> rowInd(h.nnzero);
    for (int i = 0; i < h.nnzero; ++i) {
        file >> rowInd[i];
        --rowInd[i]; // 1-based -> 0-based
    }

    // Values (optional)
    std::vector<double> val;
    if (h.valcrd > 0) {
        val.resize(h.nnzero);
        for (int i = 0; i < h.nnzero; ++i) file >> val[i];
    }
    else {
        val.assign(h.nnzero, 1.0);
    }

    for (int col = 0; col < h.ncol; ++col) {
        for (int idx = colPtr[col]; idx < colPtr[col + 1]; ++idx) {
            const int row = rowInd[idx];
            const double w = val[idx];

            if (row == col) continue;
            if (w < 0)     continue;
            if (std::abs(w) <= kMinWeightThreshold) continue;

            if (row < static_cast<int>(nodes.size()) && col < static_cast<int>(nodes.size())) {
                auto a = graph.addArc(nodes[row], nodes[col]); // directed
                weights[a] = w;
            }
        }
    }
    return true;
}

void RBToLemonConverter::saveToLemonFormat(const std::string& fn) {
    std::ofstream f(fn);
    if (!f.is_open()) {
        std::cerr << "Error: Cannot create output file " << fn << std::endl;
        return;
    }

    // Match the test format exactly:
    // @nodes: "label id"
    // @arcs: header line with "    weight" only, and rows "u v w"
    f << "@nodes\n";
    f << "label id\n";
    for (lemon::ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
        int lbl = nodeIds[n];
        int id = nodeIds[n]; // אם תרצי מזהה שונה, שימי אותו כאן
        f << lbl << " " << id << "\n";
    }
    f << "\n";

    f << "@arcs\n"; 
    f << "    weight\n";
    for (lemon::ListDigraph::ArcIt a(graph); a != lemon::INVALID; ++a) {
        const auto u = graph.source(a), v = graph.target(a);
        f << nodeIds[u] << " " << nodeIds[v] << " " << weights[a] << "\n";
    }

    std::cout << "Digraph saved to " << fn << std::endl;
    std::cout << "Nodes: " << lemon::countNodes(graph) << std::endl;
    std::cout << "Arcs: " << lemon::countArcs(graph) << std::endl;
}

void RBToLemonConverter::saveToGraphML(const std::string& fn) {
    std::ofstream f(fn);
    if (!f.is_open()) {
        std::cerr << "Error: Cannot create GraphML file " << fn << std::endl;
        return;
    }

    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    f << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\"\n";
    f << "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n";
    f << "         xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns "
        "http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">\n";
    f << "  <key id=\"weight\" for=\"edge\" attr.name=\"weight\" attr.type=\"double\"/>\n";
    f << "  <graph id=\"G\" edgedefault=\"directed\">\n"; 

    for (lemon::ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n)
        f << "    <node id=\"n" << nodeIds[n] << "\"/>\n";

    for (lemon::ListDigraph::ArcIt a(graph); a != lemon::INVALID; ++a) {
        const auto u = graph.source(a), v = graph.target(a);
        f << "    <edge source=\"n" << nodeIds[u] << "\" target=\"n" << nodeIds[v] << "\">\n";
        f << "      <data key=\"weight\">" << weights[a] << "</data>\n";
        f << "    </edge>\n";
    }

    f << "  </graph>\n</graphml>\n";

    std::cout << "GraphML (directed) saved to: " << fn << std::endl;
}

void RBToLemonConverter::printGraphStats() {
    std::cout << "\nGraph Statistics (directed):\n";
    std::cout << "Nodes: " << lemon::countNodes(graph) << "\n";
    std::cout << "Arcs:  " << lemon::countArcs(graph) << "\n";

    std::vector<int> outdeg;
    for (lemon::ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n)
        outdeg.push_back(lemon::countOutArcs(graph, n));

    if (!outdeg.empty()) {
        int mn = *std::min_element(outdeg.begin(), outdeg.end());
        int mx = *std::max_element(outdeg.begin(), outdeg.end());
        double avg = std::accumulate(outdeg.begin(), outdeg.end(), 0.0) / outdeg.size();
        std::cout << "Min out-degree: " << mn << "\n"
            << "Max out-degree: " << mx << "\n"
            << "Average out-degree: " << avg << "\n";
    }
}
