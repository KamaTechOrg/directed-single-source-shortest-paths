#include "rb_to_lemon.hpp"

using rbconv::RBHeader;
using rbconv::RBToLemonConverter;

RBToLemonConverter::RBToLemonConverter()
    : nodeIds(graph), weights(graph) {}

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
    // Create nodes
    std::vector<lemon::ListGraph::Node> nodes;
    int maxDim = std::max(h.nrow, h.ncol);
    nodes.reserve(maxDim);
    for (int i = 0; i < maxDim; ++i) {
        lemon::ListGraph::Node n = graph.addNode();
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

    // Build edges
    for (int col = 0; col < h.ncol; ++col) {
        for (int idx = colPtr[col]; idx < colPtr[col + 1]; ++idx) {
            int row = rowInd[idx];
            double w = val[idx];

            if (std::abs(w) > 1e-12) {
                if (row < (int)nodes.size() && col < (int)nodes.size()) {
                    auto e = graph.addEdge(nodes[row], nodes[col]);
                    weights[e] = w;
                }
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

    f << "# LEMON Graph Format\n";
    f << "# Converted from Rutherford-Boeing format\n\n";

    f << "@nodes\n";
    f << "label\tid\n";
    for (lemon::ListGraph::NodeIt n(graph); n != lemon::INVALID; ++n)
        f << nodeIds[n] << "\t" << nodeIds[n] << "\n";
    f << "\n";

    f << "@edges\n";
    f << "\t\tlabel\tweight\n";
    int eid = 0;
    for (lemon::ListGraph::EdgeIt e(graph); e != lemon::INVALID; ++e) {
        auto u = graph.u(e), v = graph.v(e);
        f << nodeIds[u] << "\t" << nodeIds[v] << "\t" << eid++ << "\t" << weights[e] << "\n";
    }

    std::cout << "Graph saved to " << fn << std::endl;
    std::cout << "Nodes: " << lemon::countNodes(graph) << std::endl;
    std::cout << "Edges: " << lemon::countEdges(graph) << std::endl;
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
    f << "  <graph id=\"G\" edgedefault=\"undirected\">\n";

    for (lemon::ListGraph::NodeIt n(graph); n != lemon::INVALID; ++n)
        f << "    <node id=\"n" << nodeIds[n] << "\"/>\n";

    for (lemon::ListGraph::EdgeIt e(graph); e != lemon::INVALID; ++e) {
        auto u = graph.u(e), v = graph.v(e);
        f << "    <edge source=\"n" << nodeIds[u] << "\" target=\"n" << nodeIds[v] << "\">\n";
        f << "      <data key=\"weight\">" << weights[e] << "</data>\n";
        f << "    </edge>\n";
    }

    f << "  </graph>\n</graphml>\n";

    std::cout << "Graph saved to GraphML format: " << fn << std::endl;
}

void RBToLemonConverter::printGraphStats() {
    std::cout << "\nGraph Statistics:\n";
    std::cout << "Nodes: " << lemon::countNodes(graph) << "\n";
    std::cout << "Edges: " << lemon::countEdges(graph) << "\n";

    std::vector<int> deg;
    for (lemon::ListGraph::NodeIt n(graph); n != lemon::INVALID; ++n)
        deg.push_back(lemon::countIncEdges(graph, n));

    if (!deg.empty()) {
        int mn = *std::min_element(deg.begin(), deg.end());
        int mx = *std::max_element(deg.begin(), deg.end());
        double avg = std::accumulate(deg.begin(), deg.end(), 0.0) / deg.size();
        std::cout << "Min degree: " << mn << "\n"
            << "Max degree: " << mx << "\n"
            << "Average degree: " << avg << "\n";
    }
}
