//#include <iostream>
//#include <vector>
//#include <limits>
//#include <cstddef>
//
//#include "sssp/algorithms/types.hpp"      // sssp::AdjList, sssp::BMSSPResult
//#include "sssp/algorithms/bmssp.hpp"      // הסיגנאטורה של bmssp (עדכני לשביל שלך)
//// ודאי שה-header הזה באמת כולל את bmssp שלך
//
//int main() {
//    using Key = int;
//
//    // Build a tiny directed weighted graph (AdjList)
//    // 0 -> 1 (2), 0 -> 2 (5)
//    // 1 -> 2 (1), 1 -> 3 (3)
//    // 2 -> 3 (2)
//    sssp::AdjList<Key> adj = {
//        { {1, 2.0}, {2, 5.0} },   // from 0
//        { {2, 1.0}, {3, 3.0} },   // from 1
//        { {3, 2.0} },             // from 2
//        { }                       // from 3
//    };
//
//    // Distance vector: init to +inf except source
//    std::vector<double> db(4, std::numeric_limits<double>::infinity());
//    db[0] = 0.0; // source is node 0
//
//    // index_of: Key -> index
//    auto index_of = [](Key k) -> std::size_t { return static_cast<std::size_t>(k); };
//
//    // BMSSP params
//    int l = 2;                    // recursion depth
//    double B = 10.0;              // upper bound
//    std::vector<Key> S = { 0 };     // initial set (contains the source)
//    std::size_t M = 4;            // block size (tune for your D)
//    std::size_t Ksz = 2;          // k parameter
//
//    // Run BMSSP
//    auto result = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, Ksz);
//
//    // Print results
//    std::cout << "B' = " << result.Bprime << "\n";
//    std::cout << "U  = ";
//    for (auto u : result.U) std::cout << u << " ";
//    std::cout << "\n\nDistances:\n";
//    for (std::size_t i = 0; i < db.size(); ++i) {
//        std::cout << "db[" << i << "] = " << db[i] << "\n";
//    }
//
//    // A simple expectation (manual check):
//    // Shortest 0->3 should be 0->1(2) + 1->2(1) + 2->3(2) = 5
//    // If you see db[3] ~= 5.0 you are in good shape.
//    return 0;
//}




//// src/main.cpp
//#include <iostream>
//#include <string>
//#include <vector>
//#include <utility>
//#include <limits>
//#include <chrono>
//#include <cmath>
//#include <algorithm>
//#include <unordered_map>
//
//#include "rb_to_lemon.hpp" 
//#include <lemon/list_graph.h>
//#include <lemon/lgf_reader.h>
//
//#include "sssp/algorithms/bmssp.hpp"
//
//using Key = int;
//using Adj = std::vector<std::vector<std::pair<Key, double>>>;
//static const double INF = std::numeric_limits<double>::infinity();
//
//inline Adj loadAdjFromDirectedLGF(const std::string& path, bool skipNeg = true) {
//    using Digraph = lemon::ListDigraph;
//
//    Digraph g;
//    Digraph::ArcMap<double> w(g);
//    Digraph::NodeMap<int>   nid(g, -1);
//
//    lemon::digraphReader(g, path)
//        .nodeMap("id", nid)
//        .arcMap("weight", w)
//        .run();
//
//    std::vector<int> ids;
//    ids.reserve(lemon::countNodes(g));
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
//    std::sort(ids.begin(), ids.end());
//    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
//
//    std::unordered_map<int, int> id2idx;
//    id2idx.reserve(ids.size());
//    for (int i = 0; i < (int)ids.size(); ++i) id2idx[ids[i]] = i;
//
//    Adj adj(ids.size());
//    for (Digraph::ArcIt a(g); a != lemon::INVALID; ++a) {
//        int u = id2idx[nid[g.source(a)]];
//        int v = id2idx[nid[g.target(a)]];
//        double wt = w[a];
//        if (u == v) continue;
//        if (skipNeg && wt < 0) continue;
//        adj[u].push_back({ v, wt });
//    }
//    return adj;
//}
//
//static inline void print_distance(double d) {
//    if (std::isinf(d)) std::cout << "INF";
//    else               std::cout << d;
//}
//
//int main() {
//    try {
//        // === נתיב מוחלט לקובץ ה-RB (כפי שמופיע אצלך) ===
//        const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\EAT_RS.rb";
//        const int source = 0;
//
//        std::cout << "Input RB file : " << rb_path << "\n";
//        std::cout << "Source node   : " << source << "\n";
//
//        // --- שלב 1: קריאת RB ובניית גרף LEMON ---
//        rbconv::RBToLemonConverter conv;
//        auto t0 = std::chrono::high_resolution_clock::now();
//        if (!conv.readRutherfordBoeing(rb_path)) {
//            std::cerr << "Failed to read RB file.\n";
//            return 1;
//        }
//        conv.printGraphStats();
//
//        // בסיס שם קובץ (בלי סיומת), ואז יצירת שמות לקבצי פלט
//        const std::size_t dotPos = rb_path.find_last_of('.');
//        const std::string base = (dotPos == std::string::npos) ? rb_path
//            : rb_path.substr(0, dotPos);
//        const std::string lgf_path = base + "_graph.lgf";
//        const std::string graphml_path = base + "_graph.graphml";
//
//        // --- שלב 2: שמירה ל-LGF/GraphML ---
//        conv.saveToLemonFormat(lgf_path);
//        conv.saveToGraphML(graphml_path);
//
//        // --- שלב 3: טעינת LGF ל-Adjacency ---
//        Adj adj = loadAdjFromDirectedLGF(lgf_path, /*skipNeg=*/true);
//        const std::size_t n = adj.size();
//        if (n == 0) { std::cerr << "Adjacency is empty.\n"; return 1; }
//        if (source < 0 || static_cast<std::size_t>(source) >= n) {
//            std::cerr << "Source " << source << " is out of range [0," << (n - 1) << "].\n";
//            return 1;
//        }
//
//        // --- שלב 4: הרצת BMSSP ---
//        std::vector<double> db(n, INF);
//        db[static_cast<std::size_t>(source)] = 0.0;
//        std::vector<Key> S = { source };
//
//        auto index_of = [](Key k) -> std::size_t { return static_cast<std::size_t>(k); };
//
//        int l = 2;
//        double B = INF;
//        std::size_t M = 1u << 16;   // 1u כדי לסתום אזהרות הזזה
//        std::size_t K = 1u << 16;
//
//        auto t1 = std::chrono::high_resolution_clock::now();
//        auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);
//        auto t2 = std::chrono::high_resolution_clock::now();
//
//        // --- שלב 5: סיכום ---
//        std::chrono::duration<double> convert_time = t1 - t0;
//        std::chrono::duration<double> run_time = t2 - t1;
//
//        std::size_t reachable = 0;
//        double min_finite = INF, max_finite = 0.0;
//        for (double d : db) {
//            if (!std::isinf(d)) {
//                ++reachable;
//                if (d > 0.0) min_finite = std::min(min_finite, d);
//                max_finite = std::max(max_finite, d);
//            }
//        }
//
//        std::cout << "\n===== BMSSP summary =====\n";
//        std::cout << "Nodes          : " << n << "\n";
//        std::cout << "Source         : " << source << "\n";
//        std::cout << "Reachable      : " << reachable
//            << " (" << (100.0 * static_cast<double>(reachable) / static_cast<double>(n)) << "%)\n";
//        std::cout << "Min positive d : "; print_distance(min_finite); std::cout << "\n";
//        std::cout << "Max finite d   : "; print_distance(max_finite); std::cout << "\n";
//        std::cout << "B'             : " << out.Bprime << "\n";
//        std::cout << "RB->LGF time   : " << convert_time.count() << " s\n";
//        std::cout << "BMSSP run time : " << run_time.count() << " s\n";
//        std::cout << "LGF used       : " << lgf_path << "\n";
//        std::cout << "=========================\n";
//
//        // הדפסת 20 הראשונים
//        const std::size_t show = std::min<std::size_t>(n, 20);
//        for (std::size_t i = 0; i < show; ++i) {
//            std::cout << "d[" << i << "] = ";
//            print_distance(db[i]);
//            std::cout << "\n";
//        }
//
//        return 0;
//    }
//    catch (const std::exception& ex) {
//        std::cerr << "Exception: " << ex.what() << "\n";
//        return 1;
//    }
//}




// src/main.cpp
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
#include <lemon/dijkstra.h>   // <<< חשובה להרצת דייקסטרה

#include "sssp/algorithms/bmssp.hpp"

using Key = int;
using Adj = std::vector<std::vector<std::pair<Key, double>>>;
static const double INF = std::numeric_limits<double>::infinity();

// ===== טעינת LGF ל-Adjacency (כבר היה אצלך) =====
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

// ===== הרצת Dijkstra של LEMON על אותו LGF =====
inline std::vector<double> run_lemon_dijkstra_on_lgf(
    const std::string& lgf_path,
    int source_id,            // זהה ל־source שהשתמשת בו ל-BMSSP
    std::size_t& out_n)       // גודל הווקטור המוחזר (מס' צמתים)
{
    using Digraph = lemon::ListDigraph;

    Digraph g;
    Digraph::ArcMap<double> w(g);
    Digraph::NodeMap<int>   nid(g, -1);

    lemon::digraphReader(g, lgf_path)
        .nodeMap("id", nid)
        .arcMap("weight", w)
        .run();

    // בונים את אותה מפת מזהים → אינדקס רציף כמו בפונקציה למעלה:
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

    // מאתרים את צומת המקור לפי ה-ID (לא לפי אינדקס!)
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
        // === נתיב מוחלט לקובץ ה-RB (כפי שמופיע אצלך) ===
        //const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\EAT_RS.rb";
        const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\patents_main.rb";
        const int source = 0;

        std::cout << "Input RB file : " << rb_path << "\n";
        std::cout << "Source node   : " << source << "\n";

        // --- שלב 1: קריאת RB ובניית גרף LEMON ---
        rbconv::RBToLemonConverter conv;
        auto t0 = std::chrono::high_resolution_clock::now();
        if (!conv.readRutherfordBoeing(rb_path)) {
            std::cerr << "Failed to read RB file.\n";
            return 1;
        }
        conv.printGraphStats();

        // בסיס שם קובץ (בלי סיומת), ואז יצירת שמות לקבצי פלט
        const std::size_t dotPos = rb_path.find_last_of('.');
        const std::string base = (dotPos == std::string::npos) ? rb_path
            : rb_path.substr(0, dotPos);
        const std::string lgf_path = base + "_graph.lgf";
        const std::string graphml_path = base + "_graph.graphml";

        // --- שלב 2: שמירה ל-LGF/GraphML ---
        conv.saveToLemonFormat(lgf_path);
        conv.saveToGraphML(graphml_path);

        // --- שלב 3: טעינת LGF ל-Adjacency ---
        Adj adj = loadAdjFromDirectedLGF(lgf_path, /*skipNeg=*/true);
        const std::size_t n = adj.size();
        if (n == 0) { std::cerr << "Adjacency is empty.\n"; return 1; }
        if (source < 0 || static_cast<std::size_t>(source) >= n) {
            std::cerr << "Source " << source << " is out of range [0," << (n - 1) << "].\n";
            return 1;
        }

        // --- שלב 4: הרצת BMSSP ---
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

        // --- סיכום BMSSP ---
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

        // --- שלב 5: הרצת Dijkstra של LEMON על אותו LGF ---
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

        // סיכום Dijkstra
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

        // --- שלב 6: השוואה בין BMSSP ל-Dijkstra ---
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
