
#include <iostream>
#include <string>
#include <vector>      // std::vector
#include <utility>     // std::pair
#include <algorithm>   // std::min
#include <fstream>     // std::ifstream

#include "lgf_to_adj.hpp"

using Adj = std::vector<std::vector<std::pair<int, double>>>;

static void print_summary(const Adj& adj,
    int preview_nodes = 5,
    int preview_edges_per_node = 5)
{
    const int n = static_cast<int>(adj.size());
    long long m = 0;
    for (const auto& nbrs : adj) m += static_cast<long long>(nbrs.size());

    std::cout << "Adjacency built.\n";
    std::cout << "|V| = " << n << ", |A| = " << m << " (directed)\n";

    const int showN = std::min(n, preview_nodes);
    for (int u = 0; u < showN; ++u) {
        std::cout << "u=" << u << " ->";
        int shown = 0;
        for (const auto& e : adj[u]) {
            if (shown++ >= preview_edges_per_node) { std::cout << " ..."; break; }
            int v = e.first;
            double w = e.second;
            std::cout << " (" << v << "," << w << ")";
        }
        if (adj[u].empty()) std::cout << " (no outgoing arcs)";
        std::cout << "\n";
    }
}

int main(int argc, char* argv[])
{

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-lgf-file>\n";
        return 1;
    }
    //std::string temp = R"(C:\Users\user1\Desktop\directed-single-source-shortest-paths\data\converted_graph.lgf)";
    std::string lgf = argv[1];

    std::ifstream fin(lgf);
    if (!fin.good()) {
        std::cerr << "LGF not found or cannot be opened: " << lgf << "\n";
        return 1;
    }

    try {
        Adj adj = loadAdjFromDirectedLGF(lgf, /*skipNeg=*/true);

        print_summary(adj);
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to load LGF: " << ex.what() << "\n";
        return 2;
    }
    return 0;
}
