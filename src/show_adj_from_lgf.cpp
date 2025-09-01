#include <iostream>
#include <string>
#include <algorithm>   // std::min
#include <fstream>     // std::ifstream
#include "io/sssp_lgf_io.hpp"

static void print_summary(const std::vector<std::vector<std::pair<int, double>>>& adj,
    int preview_nodes = 5, int preview_edges_per_node = 5)
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

int main() {
    // נתיב קשיח לקובץ ה-LGF שלך (עדכני אם צריך)
    std::string lgf = R"(C:\Users\user1\Desktop\directed-single-source-shortest-paths\data\converted_graph.lgf)";

    // בדיקת קיום בלי <filesystem>
    std::ifstream fin(lgf);
    if (!fin.good()) {
        std::cerr << "LGF not found or cannot be opened: " << lgf << "\n";
        return 1;
    }

    try {
        auto adj = loadAdjFromDirectedLGF(lgf, /*skipNeg=*/true);
        print_summary(adj);
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to load LGF: " << ex.what() << "\n";
        return 2;
    }
    return 0;
}
