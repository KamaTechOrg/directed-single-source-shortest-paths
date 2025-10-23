
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <limits>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <unordered_map>

#include "rb_to_lemon.hpp"

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/dijkstra.h>   

#include "sssp/algorithms/bmssp.hpp"

using Key = int;
using Adj = std::vector<std::vector<std::pair<Key, double>>>;
static const double INF = std::numeric_limits<double>::infinity();


inline Adj loadAdjFromDirectedLGF(const std::string& path, bool skipNeg = true) {
    using Digraph = lemon::ListDigraph;

    Digraph g;
    Digraph::ArcMap<double> w(g);
    Digraph::NodeMap<int>   nid(g, -1);

    lemon::digraphReader(g, path)
        .nodeMap("id", nid)
        .arcMap("weight", w)
        .run();

    std::vector<int> ids;
    ids.reserve(lemon::countNodes(g));
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    std::unordered_map<int, int> id2idx;
    id2idx.reserve(ids.size());
    for (int i = 0; i < (int)ids.size(); ++i) id2idx[ids[i]] = i;

    Adj adj(ids.size());
    for (Digraph::ArcIt a(g); a != lemon::INVALID; ++a) {
        int u = id2idx[nid[g.source(a)]];
        int v = id2idx[nid[g.target(a)]];
        double wt = w[a];
        if (u == v) continue;
        if (skipNeg && wt < 0) continue;
        adj[u].push_back({ v, wt });
    }
    return adj;
}

static inline void print_distance(double d) {
    if (std::isinf(d)) std::cout << "INF";
    else               std::cout << d;
}


inline std::vector<double> run_lemon_dijkstra_on_lgf(
    const std::string& lgf_path,
    int source_id,           
    std::size_t& out_n)       
{
    using Digraph = lemon::ListDigraph;

    Digraph g;
    Digraph::ArcMap<double> w(g);
    Digraph::NodeMap<int>   nid(g, -1);

    lemon::digraphReader(g, lgf_path)
        .nodeMap("id", nid)
        .arcMap("weight", w)
        .run();

   
    std::vector<int> ids;
    ids.reserve(lemon::countNodes(g));
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    std::unordered_map<int, int> id2idx;
    id2idx.reserve(ids.size());
    for (int i = 0; i < (int)ids.size(); ++i) id2idx[ids[i]] = i;

    out_n = ids.size();
    std::vector<double> dist(out_n, INF);
    Digraph::Node src = lemon::INVALID;
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
        if (nid[n] == source_id) { src = n; break; }
    }
    if (src == lemon::INVALID) {
        std::cerr << "Dijkstra: source id " << source_id << " not found in LGF.\n";
        return dist;
    }

    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
    dij.run(src);

    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
        int id = nid[n];
        auto it = id2idx.find(id);
        if (it == id2idx.end()) continue;
        std::size_t idx = static_cast<std::size_t>(it->second);
        if (dij.reached(n)) dist[idx] = dij.dist(n);
        else                dist[idx] = INF;
    }
    return dist;
}

int main() {
    try {
        const std::string rb_path = "../../../../data/patents_main.rb";
        const int source = 0;

        std::cout << "Input RB file : " << rb_path << "\n";
        std::cout << "Source node   : " << source << "\n";

      
        rbconv::RBToLemonConverter conv;
        auto t0 = std::chrono::high_resolution_clock::now();
        if (!conv.readRutherfordBoeing(rb_path)) {
            std::cerr << "Failed to read RB file.\n";
            return 1;
        }
        conv.printGraphStats();

    
        const std::size_t dotPos = rb_path.find_last_of('.');
        const std::string base = (dotPos == std::string::npos) ? rb_path
            : rb_path.substr(0, dotPos);
        const std::string lgf_path = base + "_graph.lgf";
        const std::string graphml_path = base + "_graph.graphml";

     
        conv.saveToLemonFormat(lgf_path);
        conv.saveToGraphML(graphml_path);

       
        Adj adj = loadAdjFromDirectedLGF(lgf_path, /*skipNeg=*/true);
        const std::size_t n = adj.size();
        if (n == 0) { std::cerr << "Adjacency is empty.\n"; return 1; }
        if (source < 0 || static_cast<std::size_t>(source) >= n) {
            std::cerr << "Source " << source << " is out of range [0," << (n - 1) << "].\n";
            return 1;
        }

       
        std::vector<double> db(n, INF);
        db[static_cast<std::size_t>(source)] = 0.0;
        std::vector<Key> S = { source };
        auto index_of = [](Key k) -> std::size_t { return static_cast<std::size_t>(k); };

        int l = 2;
        double B = INF;
        std::size_t M = 1u << 16;
        std::size_t K = 1u << 16;

        auto t1 = std::chrono::high_resolution_clock::now();
        auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);
        auto t2 = std::chrono::high_resolution_clock::now();

 
        std::chrono::duration<double> convert_time = t1 - t0;
        std::chrono::duration<double> bmssp_time = t2 - t1;

        std::size_t reachable = 0;
        double min_finite = INF, max_finite = 0.0;
        for (double d : db) {
            if (!std::isinf(d)) {
                ++reachable;
                if (d > 0.0) min_finite = std::min(min_finite, d);
                max_finite = std::max(max_finite, d);
            }
        }

        std::cout << "\n===== BMSSP summary =====\n";
        std::cout << "Nodes          : " << n << "\n";
        std::cout << "Source         : " << source << "\n";
        std::cout << "Reachable      : " << reachable
            << " (" << (100.0 * static_cast<double>(reachable) / static_cast<double>(n)) << "%)\n";
        std::cout << "Min positive d : "; print_distance(min_finite); std::cout << "\n";
        std::cout << "Max finite d   : "; print_distance(max_finite); std::cout << "\n";
        std::cout << "B'             : " << out.Bprime << "\n";
        std::cout << "RB->LGF time   : " << convert_time.count() << " s\n";
        std::cout << "BMSSP run time : " << bmssp_time.count() << " s\n";
        std::cout << "LGF used       : " << lgf_path << "\n";
        std::cout << "=========================\n";

        const std::size_t show = std::min<std::size_t>(n, 20);
        for (std::size_t i = 0; i < show; ++i) {
            std::cout << "d[" << i << "] = ";
            print_distance(db[i]);
            std::cout << "\n";
        }

       
        auto t3 = std::chrono::high_resolution_clock::now();
        std::size_t n_ref = 0;
        std::vector<double> d_ref = run_lemon_dijkstra_on_lgf(lgf_path, /*source_id=*/source, n_ref);
        auto t4 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> dijkstra_time = t4 - t3;

        if (n_ref != n) {
            std::cerr << "[WARN] Dijkstra vector size (" << n_ref
                << ") differs from BMSSP (" << n << "). Comparing min(n).\n";
        }
        const std::size_t cmpN = std::min(n_ref, n);

       
        std::size_t reachable_ref = 0;
        double min_finite_ref = INF, max_finite_ref = 0.0;
        for (std::size_t i = 0; i < cmpN; ++i) {
            if (!std::isinf(d_ref[i])) {
                ++reachable_ref;
                if (d_ref[i] > 0.0) min_finite_ref = std::min(min_finite_ref, d_ref[i]);
                max_finite_ref = std::max(max_finite_ref, d_ref[i]);
            }
        }

        std::cout << "\n===== Dijkstra (LEMON) summary =====\n";
        std::cout << "Nodes          : " << cmpN << "\n";
        std::cout << "Source         : " << source << "\n";
        std::cout << "Reachable      : " << reachable_ref
            << " (" << (100.0 * static_cast<double>(reachable_ref) / static_cast<double>(cmpN)) << "%)\n";
        std::cout << "Min positive d : "; print_distance(min_finite_ref); std::cout << "\n";
        std::cout << "Max finite d   : "; print_distance(max_finite_ref); std::cout << "\n";
        std::cout << "Dijkstra time  : " << dijkstra_time.count() << " s\n";
        std::cout << "LGF used       : " << lgf_path << "\n";
        std::cout << "=========================\n";

        for (std::size_t i = 0; i < std::min<std::size_t>(cmpN, 20); ++i) {
            std::cout << "d_ref[" << i << "] = ";
            print_distance(d_ref[i]);
            std::cout << "\n";
        }

      
        std::size_t mismatches = 0;
        double max_abs_diff = 0.0;
        const double EPS = 1e-9;

        for (std::size_t i = 0; i < cmpN; ++i) {
            const bool a_inf = std::isinf(db[i]);
            const bool b_inf = std::isinf(d_ref[i]);

            if (a_inf != b_inf) {
                ++mismatches;
                continue;
            }
            if (!a_inf && !b_inf) {
                double diff = std::fabs(db[i] - d_ref[i]);
                max_abs_diff = std::max(max_abs_diff, diff);
                if (diff > EPS) ++mismatches;
            }
        }

        std::cout << "\n===== Comparison (BMSSP vs Dijkstra) =====\n";
        std::cout << "Compared nodes : " << cmpN << "\n";
        std::cout << "Mismatches     : " << mismatches << "\n";
        std::cout << "Max Abs Diff       : " << max_abs_diff << "\n";
        std::cout << "==========================================\n";

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << "\n";
        return 1;
    }
}
