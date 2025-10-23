

//מקור תמיד קודקוד 0
// src/main.cpp
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
//
//#include <lemon/list_graph.h>
//#include <lemon/lgf_reader.h>
//#include <lemon/dijkstra.h>   // <<< חשובה להרצת דייקסטרה
//
//#include "sssp/algorithms/bmssp.hpp"
//
//using Key = int;
//using Adj = std::vector<std::vector<std::pair<Key, double>>>;
//static const double INF = std::numeric_limits<double>::infinity();
//
//// ===== טעינת LGF ל-Adjacency (כבר היה אצלך) =====
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
//// ===== הרצת Dijkstra של LEMON על אותו LGF =====
//inline std::vector<double> run_lemon_dijkstra_on_lgf(
//    const std::string& lgf_path,
//    int source_id,            // זהה ל־source שהשתמשת בו ל-BMSSP
//    std::size_t& out_n)       // גודל הווקטור המוחזר (מס' צמתים)
//{
//    using Digraph = lemon::ListDigraph;
//
//    Digraph g;
//    Digraph::ArcMap<double> w(g);
//    Digraph::NodeMap<int>   nid(g, -1);
//
//    lemon::digraphReader(g, lgf_path)
//        .nodeMap("id", nid)
//        .arcMap("weight", w)
//        .run();
//
//    // בונים את אותה מפת מזהים → אינדקס רציף כמו בפונקציה למעלה:
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
//    out_n = ids.size();
//    std::vector<double> dist(out_n, INF);
//
//    // מאתרים את צומת המקור לפי ה-ID (לא לפי אינדקס!)
//    Digraph::Node src = lemon::INVALID;
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
//        if (nid[n] == source_id) { src = n; break; }
//    }
//    if (src == lemon::INVALID) {
//        std::cerr << "Dijkstra: source id " << source_id << " not found in LGF.\n";
//        return dist;
//    }
//
//    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
//    dij.run(src);
//
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
//        int id = nid[n];
//        auto it = id2idx.find(id);
//        if (it == id2idx.end()) continue;
//        std::size_t idx = static_cast<std::size_t>(it->second);
//        if (dij.reached(n)) dist[idx] = dij.dist(n);
//        else                dist[idx] = INF;
//    }
//    return dist;
//}
//
//int main() {
//    try {
//        // === נתיב מוחלט לקובץ ה-RB (כפי שמופיע אצלך) ===
//        //const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\EAT_RS.rb";
//        const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\patents_main.rb";
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
//        auto index_of = [](Key k) -> std::size_t { return static_cast<std::size_t>(k); };
//
//        int l = 2;
//        double B = INF;
//        std::size_t M = 1u << 16;
//        std::size_t K = 1u << 16;
//
//        auto t1 = std::chrono::high_resolution_clock::now();
//        auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);
//        auto t2 = std::chrono::high_resolution_clock::now();
//
//        // --- סיכום BMSSP ---
//        std::chrono::duration<double> convert_time = t1 - t0;
//        std::chrono::duration<double> bmssp_time = t2 - t1;
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
//        std::cout << "BMSSP run time : " << bmssp_time.count() << " s\n";
//        std::cout << "LGF used       : " << lgf_path << "\n";
//        std::cout << "=========================\n";
//
//        const std::size_t show = std::min<std::size_t>(n, 20);
//        for (std::size_t i = 0; i < show; ++i) {
//            std::cout << "d[" << i << "] = ";
//            print_distance(db[i]);
//            std::cout << "\n";
//        }
//
//        // --- שלב 5: הרצת Dijkstra של LEMON על אותו LGF ---
//        auto t3 = std::chrono::high_resolution_clock::now();
//        std::size_t n_ref = 0;
//        std::vector<double> d_ref = run_lemon_dijkstra_on_lgf(lgf_path, /*source_id=*/source, n_ref);
//        auto t4 = std::chrono::high_resolution_clock::now();
//        std::chrono::duration<double> dijkstra_time = t4 - t3;
//
//        if (n_ref != n) {
//            std::cerr << "[WARN] Dijkstra vector size (" << n_ref
//                << ") differs from BMSSP (" << n << "). Comparing min(n).\n";
//        }
//        const std::size_t cmpN = std::min(n_ref, n);
//
//        // סיכום Dijkstra
//        std::size_t reachable_ref = 0;
//        double min_finite_ref = INF, max_finite_ref = 0.0;
//        for (std::size_t i = 0; i < cmpN; ++i) {
//            if (!std::isinf(d_ref[i])) {
//                ++reachable_ref;
//                if (d_ref[i] > 0.0) min_finite_ref = std::min(min_finite_ref, d_ref[i]);
//                max_finite_ref = std::max(max_finite_ref, d_ref[i]);
//            }
//        }
//
//        std::cout << "\n===== Dijkstra (LEMON) summary =====\n";
//        std::cout << "Nodes          : " << cmpN << "\n";
//        std::cout << "Source         : " << source << "\n";
//        std::cout << "Reachable      : " << reachable_ref
//            << " (" << (100.0 * static_cast<double>(reachable_ref) / static_cast<double>(cmpN)) << "%)\n";
//        std::cout << "Min positive d : "; print_distance(min_finite_ref); std::cout << "\n";
//        std::cout << "Max finite d   : "; print_distance(max_finite_ref); std::cout << "\n";
//        std::cout << "Dijkstra time  : " << dijkstra_time.count() << " s\n";
//        std::cout << "LGF used       : " << lgf_path << "\n";
//        std::cout << "=========================\n";
//
//        for (std::size_t i = 0; i < std::min<std::size_t>(cmpN, 20); ++i) {
//            std::cout << "d_ref[" << i << "] = ";
//            print_distance(d_ref[i]);
//            std::cout << "\n";
//        }
//
//        // --- שלב 6: השוואה בין BMSSP ל-Dijkstra ---
//        std::size_t mismatches = 0;
//        double max_abs_diff = 0.0;
//        const double EPS = 1e-9;
//
//        for (std::size_t i = 0; i < cmpN; ++i) {
//            const bool a_inf = std::isinf(db[i]);
//            const bool b_inf = std::isinf(d_ref[i]);
//
//            if (a_inf != b_inf) {
//                ++mismatches;
//                continue;
//            }
//            if (!a_inf && !b_inf) {
//                double diff = std::fabs(db[i] - d_ref[i]);
//                max_abs_diff = std::max(max_abs_diff, diff);
//                if (diff > EPS) ++mismatches;
//            }
//        }
//
//        std::cout << "\n===== Comparison (BMSSP vs Dijkstra) =====\n";
//        std::cout << "Compared nodes : " << cmpN << "\n";
//        std::cout << "Mismatches     : " << mismatches << "\n";
//        std::cout << "Max Abs Diff       : " << max_abs_diff << "\n";
//        std::cout << "==========================================\n";
//
//        return 0;
//    }
//    catch (const std::exception& ex) {
//        std::cerr << "Exception: " << ex.what() << "\n";
//        return 1;
//    }
//}







////בחירת מקור שמצליח להגיע לקודקודים רבים
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
//#include <queue>            // <<< חדש
//
//#include "rb_to_lemon.hpp"
//
//#include <lemon/list_graph.h>
//#include <lemon/lgf_reader.h>
//#include <lemon/dijkstra.h>
//
//#include "sssp/algorithms/bmssp.hpp"
//
//using Key = int;
//using Adj = std::vector<std::vector<std::pair<Key, double>>>;
//static const double INF = std::numeric_limits<double>::infinity();
//
//
//// ===== טעינת LGF ל-Adjacency + החזרת מיפוי idx->id =====
//inline Adj loadAdjFromDirectedLGF(const std::string& path,
//    std::vector<int>* out_idx2id = nullptr,
//    bool skipNeg = true) {
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
//    if (out_idx2id) *out_idx2id = ids;   // <<< החזרת המיפוי idx->id
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
//// ----- עזר: BFS לספירת reachable (מתעלם ממשקלים) -----
//inline std::size_t reachable_count(const Adj& adj, int s) {
//    std::vector<char> vis(adj.size(), 0);
//    std::vector<int> q;
//    q.reserve(1024);
//    q.push_back(s);
//    vis[s] = 1;
//
//    for (std::size_t i = 0; i < q.size(); ++i) {
//        int u = q[i];
//        for (const auto& e : adj[u]) {
//            const int v = e.first;
//            if (!vis[v]) { vis[v] = 1; q.push_back(v); }
//        }
//    }
//    return q.size();
//}
//
//// ----- עזר: חיפוש K מקורות עם דרגת יציאה הכי גבוהה -----
//inline std::vector<int> top_k_by_outdeg(const Adj& adj, std::size_t K = 10) {
//    if (K == 0) K = 1;
//    if (K > adj.size()) K = adj.size();
//    // נשמור רשימת (degree, index) בגודל עד K; מתחזקים את המינימום להחלפה
//    std::vector<std::pair<std::size_t, int>> best;
//    best.reserve(K);
//
//    auto push_candidate = [&](std::size_t deg, int idx) {
//        if (best.size() < K) {
//            best.emplace_back(deg, idx);
//        }
//        else {
//            // מצא מינימלי ב-best
//            std::size_t min_pos = 0;
//            for (std::size_t i = 1; i < best.size(); ++i)
//                if (best[i].first < best[min_pos].first) min_pos = i;
//            if (deg > best[min_pos].first) best[min_pos] = { deg, idx };
//        }
//        };
//
//    for (std::size_t i = 0; i < adj.size(); ++i) {
//        push_candidate(adj[i].size(), static_cast<int>(i));
//    }
//    // מיון יורד לפי דרגה
//    std::sort(best.begin(), best.end(), [](auto& a, auto& b) {
//        if (a.first != b.first) return a.first > b.first;
//        return a.second < b.second;
//        });
//
//    std::vector<int> res;
//    res.reserve(best.size());
//    for (auto& p : best) res.push_back(p.second);
//    return res;
//}
//
//// ----- בחירת מקור: מחפשים אחד שמגיע לפחות לסף נתון (אחוז מהגרף) -----
//inline int choose_source_by_reachability(const Adj& adj,
//    double min_ratio = 0.10, // 10%
//    std::size_t K = 10,
//    std::size_t* out_reach = nullptr,
//    std::size_t* out_outdeg = nullptr) {
//    const std::size_t n = adj.size();
//    auto candidates = top_k_by_outdeg(adj, K);
//
//    std::size_t best_reach = 0;
//    int best_idx = candidates.empty() ? 0 : candidates[0];
//    std::size_t best_outdeg = candidates.empty() ? 0 : adj[best_idx].size();
//
//    for (int s : candidates) {
//        std::size_t r = reachable_count(adj, s);
//        if (r > best_reach) {
//            best_reach = r;
//            best_idx = s;
//            best_outdeg = adj[s].size();
//        }
//        if (static_cast<double>(r) / static_cast<double>(n) >= min_ratio) break; // מצאנו מספיק טוב
//    }
//
//    if (out_reach)  *out_reach = best_reach;
//    if (out_outdeg) *out_outdeg = best_outdeg;
//    return best_idx;
//}
//
//
//// ===== הרצת Dijkstra של LEMON על אותו LGF (source_id = מזהה מקורי) =====
//inline std::vector<double> run_lemon_dijkstra_on_lgf(
//    const std::string& lgf_path,
//    int source_id,
//    std::size_t& out_n)
//{
//    using Digraph = lemon::ListDigraph;
//
//    Digraph g;
//    Digraph::ArcMap<double> w(g);
//    Digraph::NodeMap<int>   nid(g, -1); // (אם עולה אזהרה MSVC, החזירי ל-NodeMap<int> )
//
//    lemon::digraphReader(g, lgf_path)
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
//    out_n = ids.size();
//    std::vector<double> dist(out_n, INF);
//
//    Digraph::Node src = lemon::INVALID;
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
//        if (nid[n] == source_id) { src = n; break; }
//    }
//    if (src == lemon::INVALID) {
//        std::cerr << "Dijkstra: source id " << source_id << " not found in LGF.\n";
//        return dist;
//    }
//
//    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
//    dij.run(src);
//
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
//        int id = nid[n];
//        auto it = id2idx.find(id);
//        if (it == id2idx.end()) continue;
//        std::size_t idx = static_cast<std::size_t>(it->second);
//        dist[idx] = dij.reached(n) ? dij.dist(n) : INF;
//    }
//    return dist;
//}
//
//
//int main() {
//    try {
//        // === נתיב מוחלט לקובץ ה-RB ===
//        // const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\EAT_RS.rb";
//        const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\patents_main.rb";
//
//        std::cout << "Input RB file : " << rb_path << "\n";
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
//        const std::size_t dotPos = rb_path.find_last_of('.');
//        const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
//        const std::string lgf_path = base + "_graph.lgf";
//        const std::string graphml_path = base + "_graph.graphml";
//
//        conv.saveToLemonFormat(lgf_path);
//        conv.saveToGraphML(graphml_path);
//
//        // --- שלב 3: טעינת LGF ל-Adjacency + מיפוי idx->id ---
//        std::vector<int> idx2id;
//        Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
//        const std::size_t n = adj.size();
//        if (n == 0) { std::cerr << "Adjacency is empty.\n"; return 1; }
//
//        // === בחירת מקור אוטומטית ===
//        std::size_t reach_est = 0, src_outdeg = 0;
//        int source_idx = choose_source_by_reachability(adj, /*min_ratio=*/0.10, /*K=*/10,
//            &reach_est, &src_outdeg);
//        int source_id = idx2id[source_idx];
//
//        std::cout << "Chosen source (index): " << source_idx
//            << " | (LGF id): " << source_id
//            << " | outdeg: " << src_outdeg
//            << " | reachable_est: " << reach_est
//            << " (" << (100.0 * static_cast<double>(reach_est) / static_cast<double>(n)) << "%)\n";
//
//        // --- שלב 4: הרצת BMSSP ---
//        std::vector<double> db(n, INF);
//        db[static_cast<std::size_t>(source_idx)] = 0.0;
//        std::vector<Key> S = { source_idx };
//        auto index_of = [](Key k) -> std::size_t { return static_cast<std::size_t>(k); };
//
//        int l = 2;
//        double B = INF;
//        std::size_t M = 1u << 16;
//        std::size_t K = 1u << 16;
//
//        auto t1 = std::chrono::high_resolution_clock::now();
//        auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);
//        auto t2 = std::chrono::high_resolution_clock::now();
//
//        // --- סיכום BMSSP ---
//        std::chrono::duration<double> convert_time = t1 - t0;
//        std::chrono::duration<double> bmssp_time = t2 - t1;
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
//        std::cout << "Source (index) : " << source_idx << " | (LGF id): " << source_id << "\n";
//        std::cout << "Reachable      : " << reachable
//            << " (" << (100.0 * static_cast<double>(reachable) / static_cast<double>(n)) << "%)\n";
//        std::cout << "Min positive d : "; print_distance(min_finite); std::cout << "\n";
//        std::cout << "Max finite d   : "; print_distance(max_finite); std::cout << "\n";
//        std::cout << "B'             : " << out.Bprime << "\n";
//        std::cout << "RB->LGF time   : " << convert_time.count() << " s\n";
//        std::cout << "BMSSP run time : " << bmssp_time.count() << " s\n";
//        std::cout << "LGF used       : " << lgf_path << "\n";
//        std::cout << "=========================\n";
//
//        const std::size_t show = std::min<std::size_t>(n, 20);
//        for (std::size_t i = 0; i < show; ++i) {
//            std::cout << "d[" << i << "] = "; print_distance(db[i]); std::cout << "\n";
//        }
//
//        // --- שלב 5: Dijkstra על אותו LGF, עם ה-ID המקורי של המקור ---
//        auto t3 = std::chrono::high_resolution_clock::now();
//        std::size_t n_ref = 0;
//        std::vector<double> d_ref = run_lemon_dijkstra_on_lgf(lgf_path, /*source_id=*/source_id, n_ref);
//        auto t4 = std::chrono::high_resolution_clock::now();
//        std::chrono::duration<double> dijkstra_time = t4 - t3;
//
//        if (n_ref != n) {
//            std::cerr << "[WARN] Dijkstra vector size (" << n_ref
//                << ") differs from BMSSP (" << n << "). Comparing min(n).\n";
//        }
//        const std::size_t cmpN = std::min(n_ref, n);
//
//        std::size_t reachable_ref = 0;
//        double min_finite_ref = INF, max_finite_ref = 0.0;
//        for (std::size_t i = 0; i < cmpN; ++i) {
//            if (!std::isinf(d_ref[i])) {
//                ++reachable_ref;
//                if (d_ref[i] > 0.0) min_finite_ref = std::min(min_finite_ref, d_ref[i]);
//                max_finite_ref = std::max(max_finite_ref, d_ref[i]);
//            }
//        }
//
//        std::cout << "\n===== Dijkstra (LEMON) summary =====\n";
//        std::cout << "Nodes          : " << cmpN << "\n";
//        std::cout << "Source (index) : " << source_idx << " | (LGF id): " << source_id << "\n";
//        std::cout << "Reachable      : " << reachable_ref
//            << " (" << (100.0 * static_cast<double>(reachable_ref) / static_cast<double>(cmpN)) << "%)\n";
//        std::cout << "Min positive d : "; print_distance(min_finite_ref); std::cout << "\n";
//        std::cout << "Max finite d   : "; print_distance(max_finite_ref); std::cout << "\n";
//        std::cout << "Dijkstra time  : " << dijkstra_time.count() << " s\n";
//        std::cout << "LGF used       : " << lgf_path << "\n";
//        std::cout << "=========================\n";
//
//        for (std::size_t i = 0; i < std::min<std::size_t>(cmpN, 20); ++i) {
//            std::cout << "d_ref[" << i << "] = "; print_distance(d_ref[i]); std::cout << "\n";
//        }
//
//        // --- שלב 6: השוואה בין BMSSP ל-Dijkstra ---
//        std::size_t mismatches = 0;
//        double max_abs_diff = 0.0;
//        const double EPS = 1e-9;
//
//        for (std::size_t i = 0; i < cmpN; ++i) {
//            const bool a_inf = std::isinf(db[i]);
//            const bool b_inf = std::isinf(d_ref[i]);
//
//            if (a_inf != b_inf) {
//                ++mismatches;
//            }
//            else if (!a_inf) {
//                double diff = std::fabs(db[i] - d_ref[i]);
//                max_abs_diff = std::max(max_abs_diff, diff);
//                if (diff > EPS) ++mismatches;
//            }
//        }
//
//        std::cout << "\n===== Comparison (BMSSP vs Dijkstra) =====\n";
//        std::cout << "Compared nodes : " << cmpN << "\n";
//        std::cout << "Mismatches     : " << mismatches << "\n";
//        std::cout << "Max Abs Diff   : " << max_abs_diff << "\n";
//        std::cout << "==========================================\n";
//
//        return 0;
//    }
//    catch (const std::exception& ex) {
//        std::cerr << "Exception: " << ex.what() << "\n";
//        return 1;
//    }
//}




// src/main.cpp
// בחירת קודקוד מקור + חישוב פרמטרים לפי המאמר, עם הפרדה בין k_paper ל-K_work

//#include <iostream>
//#include <string>
//#include <vector>
//#include <utility>
//#include <limits>
//#include <chrono>
//#include <cmath>
//#include <algorithm>
//#include <unordered_map>
//#include <queue>
//
//#include "rb_to_lemon.hpp"
//
//#include <lemon/list_graph.h>
//#include <lemon/lgf_reader.h>
//#include <lemon/dijkstra.h>
//
//#include "sssp/algorithms/bmssp.hpp"
//
//using Key = int;
//using Adj = std::vector<std::vector<std::pair<Key, double>>>;
//static const double INF = std::numeric_limits<double>::infinity();
//
//
//// ===== טעינת LGF ל-Adjacency + החזרת מיפוי idx->id =====
//inline Adj loadAdjFromDirectedLGF(const std::string& path,
//    std::vector<int>* out_idx2id = nullptr,
//    bool skipNeg = true) {
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
//    if (out_idx2id) *out_idx2id = ids;   // מיפוי אינדקס→ID מקורי
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
//// ----- עזר: BFS לספירת reachable (מתעלם ממשקלים) -----
//inline std::size_t reachable_count(const Adj& adj, int s) {
//    std::vector<char> vis(adj.size(), 0);
//    std::vector<int> q;
//    q.reserve(1024);
//    q.push_back(s);
//    vis[s] = 1;
//
//    for (std::size_t i = 0; i < q.size(); ++i) {
//        int u = q[i];
//        for (const auto& e : adj[u]) {
//            const int v = e.first;
//            if (!vis[v]) { vis[v] = 1; q.push_back(v); }
//        }
//    }
//    return q.size();
//}
//
//// ----- עזר: חיפוש K מקורות עם דרגת יציאה הכי גבוהה -----
//inline std::vector<int> top_k_by_outdeg(const Adj& adj, std::size_t K = 10) {
//    if (K == 0) K = 1;
//    if (K > adj.size()) K = adj.size();
//
//    std::vector<std::pair<std::size_t, int>> best;
//    best.reserve(K);
//
//    auto push_candidate = [&](std::size_t deg, int idx) {
//        if (best.size() < K) {
//            best.emplace_back(deg, idx);
//        }
//        else {
//            std::size_t min_pos = 0;
//            for (std::size_t i = 1; i < best.size(); ++i)
//                if (best[i].first < best[min_pos].first) min_pos = i;
//            if (deg > best[min_pos].first) best[min_pos] = { deg, idx };
//        }
//        };
//
//    for (std::size_t i = 0; i < adj.size(); ++i) {
//        push_candidate(adj[i].size(), static_cast<int>(i));
//    }
//
//    std::sort(best.begin(), best.end(), [](const auto& a, const auto& b) {
//        if (a.first != b.first) return a.first > b.first;
//        return a.second < b.second;
//        });
//
//    std::vector<int> res;
//    res.reserve(best.size());
//    for (const auto& p : best) res.push_back(p.second);
//    return res;
//}
//
//// ----- בחירת מקור: מחפשים אחד שמגיע לפחות לסף נתון (אחוז מהגרף) -----
//inline int choose_source_by_reachability(const Adj& adj,
//    double min_ratio = 0.10, // 10%
//    std::size_t K = 10,
//    std::size_t* out_reach = nullptr,
//    std::size_t* out_outdeg = nullptr) {
//    const std::size_t n = adj.size();
//    auto candidates = top_k_by_outdeg(adj, K);
//
//    std::size_t best_reach = 0;
//    int best_idx = candidates.empty() ? 0 : candidates[0];
//    std::size_t best_outdeg = candidates.empty() ? 0 : adj[best_idx].size();
//
//    for (int s : candidates) {
//        std::size_t r = reachable_count(adj, s);
//        if (r > best_reach) {
//            best_reach = r;
//            best_idx = s;
//            best_outdeg = adj[s].size();
//        }
//        if (static_cast<double>(r) / static_cast<double>(n) >= min_ratio) break; // מספיק טוב
//    }
//
//    if (out_reach)  *out_reach = best_reach;
//    if (out_outdeg) *out_outdeg = best_outdeg;
//    return best_idx;
//}
//
//// ===== הרצת Dijkstra של LEMON על אותו LGF (source_id = מזהה מקורי) =====
//inline std::vector<double> run_lemon_dijkstra_on_lgf(
//    const std::string& lgf_path,
//    int source_id,
//    std::size_t& out_n)
//{
//    using Digraph = lemon::ListDigraph;
//
//    Digraph g;
//    Digraph::ArcMap<double> w(g);
//    Digraph::NodeMap<int>   nid(g, -1);
//
//    lemon::digraphReader(g, lgf_path)
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
//    out_n = ids.size();
//    std::vector<double> dist(out_n, INF);
//
//    Digraph::Node src = lemon::INVALID;
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
//        if (nid[n] == source_id) { src = n; break; }
//    }
//    if (src == lemon::INVALID) {
//        std::cerr << "Dijkstra: source id " << source_id << " not found in LGF.\n";
//        return dist;
//    }
//
//    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
//    dij.run(src);
//
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
//        int id = nid[n];
//        auto it = id2idx.find(id);
//        if (it == id2idx.end()) continue;
//        std::size_t idx = static_cast<std::size_t>(it->second);
//        dist[idx] = dij.reached(n) ? dij.dist(n) : INF;
//    }
//    return dist;
//}
//
//// ===== פרמטרים לפי המאמר: k = floor((ln n)^(1/3)), t = floor((ln n)^(2/3)),
////       l = ceil(ln n / t),  M = 2^((l-1)*t) =====
//struct PaperParams {
//    int l, k_paper, t;
//    std::size_t M;
//};
//
//inline PaperParams choose_params_from_paper(std::size_t n) {
//    if (n < 2) {
//        PaperParams p{}; p.l = 1; p.k_paper = 1; p.t = 1; p.M = 1;
//        return p;
//    }
//    const double ln_n = std::log(static_cast<double>(n));         // log טבעי
//    int k = static_cast<int>(std::floor(std::cbrt(ln_n)));         // floor((ln n)^(1/3))
//    if (k < 1) k = 1;
//    int t = static_cast<int>(std::floor(std::pow(ln_n, 2.0 / 3.0))); // floor((ln n)^(2/3))
//    if (t < 1) t = 1;
//    int l = static_cast<int>(std::ceil(ln_n / static_cast<double>(t))); // ceil(ln n / t)
//    if (l < 1) l = 1;
//
//    const int exp_int = (l - 1) * t;                                  // (l-1)*t
//    const double exp_arg = static_cast<double>(std::max(0, exp_int));
//    std::size_t M = static_cast<std::size_t>(std::pow(2.0, exp_arg)); // 2^((l-1)*t)
//    if (M == 0) M = 1;
//
//    return { l, k, t, M };
//}
//
//
//int main() {
//    try {
//        // === נתיב מוחלט לקובץ ה-RB ===
//        // const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\EAT_RS.rb";
//        const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\patents_main.rb";
//
//        std::cout << "Input RB file : " << rb_path << "\n";
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
//        const std::size_t dotPos = rb_path.find_last_of('.');
//        const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
//        const std::string lgf_path = base + "_graph.lgf";
//        const std::string graphml_path = base + "_graph.graphml";
//
//        conv.saveToLemonFormat(lgf_path);
//        conv.saveToGraphML(graphml_path);
//
//        // --- שלב 3: טעינת LGF ל-Adjacency + מיפוי idx->id ---
//        std::vector<int> idx2id;
//        Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
//        const std::size_t n = adj.size();
//        if (n == 0) { std::cerr << "Adjacency is empty.\n"; return 1; }
//
//        // === בחירת מקור אוטומטית ===
//        std::size_t reach_est = 0, src_outdeg = 0;
//        int source_idx = choose_source_by_reachability(adj, /*min_ratio=*/0.10, /*K=*/10,
//            &reach_est, &src_outdeg);
//        int source_id = idx2id[source_idx];
//
//        std::cout << "Chosen source (index): " << source_idx
//            << " | (LGF id): " << source_id
//            << " | outdeg: " << src_outdeg
//            << " | reachable_est: " << reach_est
//            << " (" << (100.0 * static_cast<double>(reach_est) / static_cast<double>(n)) << "%)\n";
//
//        // === פרמטרים לפי המאמר + הפרדה לשמות ===
//        PaperParams prm = choose_params_from_paper(n);
//        const int         l = prm.l;
//        const int         k_paper = prm.k_paper;              // מהמאמר
//        const std::size_t M = prm.M;
//        const std::size_t K_work = std::max<std::size_t>(M, 1u << 16); // מגבלת עבודה (לא k_paper!)
//
//        std::cout << "Params (paper): n=" << n
//            << " | k_paper=" << k_paper
//            << " | t=" << prm.t
//            << " | l=" << l
//            << " | M=" << M << "\n";
//        std::cout << "Exec params  : K_work=" << K_work << " (passed as K to bmssp)\n";
//
//        // --- שלב 4: הרצת BMSSP ---
//        std::vector<double> db(n, INF);
//        db[static_cast<std::size_t>(source_idx)] = 0.0;
//        std::vector<Key> S = { source_idx };
//        auto index_of = [](Key k) -> std::size_t { return static_cast<std::size_t>(k); };
//
//        double B = INF;
//        auto t1 = std::chrono::high_resolution_clock::now();
//        // שימי לב: אנחנו מעבירים את K_work לפרמטר K של bmssp — לא את k_paper
//        auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K_work);
//        auto t2 = std::chrono::high_resolution_clock::now();
//
//        // --- סיכום BMSSP ---
//        std::chrono::duration<double> convert_time = t1 - t0;
//        std::chrono::duration<double> bmssp_time = t2 - t1;
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
//        std::cout << "Source (index) : " << source_idx << " | (LGF id): " << source_id << "\n";
//        std::cout << "Reachable      : " << reachable
//            << " (" << (100.0 * static_cast<double>(reachable) / static_cast<double>(n)) << "%)\n";
//        std::cout << "Min positive d : "; print_distance(min_finite); std::cout << "\n";
//        std::cout << "Max finite d   : "; print_distance(max_finite); std::cout << "\n";
//        std::cout << "B'             : " << out.Bprime << "\n";
//        std::cout << "RB->LGF time   : " << convert_time.count() << " s\n";
//        std::cout << "BMSSP run time : " << bmssp_time.count() << " s\n";
//        std::cout << "LGF used       : " << lgf_path << "\n";
//        std::cout << "=========================\n";
//
//        const std::size_t show = std::min<std::size_t>(n, 20);
//        for (std::size_t i = 0; i < show; ++i) {
//            std::cout << "d[" << i << "] = "; print_distance(db[i]); std::cout << "\n";
//        }
//
//        // --- שלב 5: Dijkstra על אותו LGF, עם ה-ID המקורי של המקור ---
//        auto t3 = std::chrono::high_resolution_clock::now();
//        std::size_t n_ref = 0;
//        std::vector<double> d_ref = run_lemon_dijkstra_on_lgf(lgf_path, /*source_id=*/source_id, n_ref);
//        auto t4 = std::chrono::high_resolution_clock::now();
//        std::chrono::duration<double> dijkstra_time = t4 - t3;
//
//        if (n_ref != n) {
//            std::cerr << "[WARN] Dijkstra vector size (" << n_ref
//                << ") differs from BMSSP (" << n << "). Comparing min(n).\n";
//        }
//        const std::size_t cmpN = std::min(n_ref, n);
//
//        std::size_t reachable_ref = 0;
//        double min_finite_ref = INF, max_finite_ref = 0.0;
//        for (std::size_t i = 0; i < cmpN; ++i) {
//            if (!std::isinf(d_ref[i])) {
//                ++reachable_ref;
//                if (d_ref[i] > 0.0) min_finite_ref = std::min(min_finite_ref, d_ref[i]);
//                max_finite_ref = std::max(max_finite_ref, d_ref[i]);
//            }
//        }
//
//        std::cout << "\n===== Dijkstra (LEMON) summary =====\n";
//        std::cout << "Nodes          : " << cmpN << "\n";
//        std::cout << "Source (index) : " << source_idx << " | (LGF id): " << source_id << "\n";
//        std::cout << "Reachable      : " << reachable_ref
//            << " (" << (100.0 * static_cast<double>(reachable_ref) / static_cast<double>(cmpN)) << "%)\n";
//        std::cout << "Min positive d : "; print_distance(min_finite_ref); std::cout << "\n";
//        std::cout << "Max finite d   : "; print_distance(max_finite_ref); std::cout << "\n";
//        std::cout << "Dijkstra time  : " << dijkstra_time.count() << " s\n";
//        std::cout << "LGF used       : " << lgf_path << "\n";
//        std::cout << "=========================\n";
//
//        for (std::size_t i = 0; i < std::min<std::size_t>(cmpN, 20); ++i) {
//            std::cout << "d_ref[" << i << "] = "; print_distance(d_ref[i]); std::cout << "\n";
//        }
//
//        // --- שלב 6: השוואה בין BMSSP ל-Dijkstra ---
//        std::size_t mismatches = 0;
//        double max_abs_diff = 0.0;
//        const double EPS = 1e-9;
//
//        for (std::size_t i = 0; i < cmpN; ++i) {
//            const bool a_inf = std::isinf(db[i]);
//            const bool b_inf = std::isinf(d_ref[i]);
//
//            if (a_inf != b_inf) {
//                ++mismatches;
//            }
//            else if (!a_inf) {
//                double diff = std::fabs(db[i] - d_ref[i]);
//                max_abs_diff = std::max(max_abs_diff, diff);
//                if (diff > EPS) ++mismatches;
//            }
//        }
//
//        std::cout << "\n===== Comparison (BMSSP vs Dijkstra) =====\n";
//        std::cout << "Compared nodes : " << cmpN << "\n";
//        std::cout << "Mismatches     : " << mismatches << "\n";
//        std::cout << "Max Abs Diff   : " << max_abs_diff << "\n";
//        std::cout << "==========================================\n";
//
//        return 0;
//    }
//    catch (const std::exception& ex) {
//        std::cerr << "Exception: " << ex.what() << "\n";
//        return 1;
//    }
//}





////כולל מדידת זמן הוגנת של ממש
//// main.cpp — BMSSP vs Dijkstra: fair timing (build vs run), no copying of LEMON types
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
//#include "sssp/algorithms/bmssp.hpp"       
//
//#include <lemon/list_graph.h>
//#include <lemon/lgf_reader.h>
//#include <lemon/dijkstra.h>
//
//using clk = std::chrono::steady_clock;
//
//using Key = int;
//using Adj = std::vector<std::vector<std::pair<Key, double>>>;
//static const double INF = std::numeric_limits<double>::infinity();
//
//// ===== עזר: הדפסת מרחק =====
//static inline void print_distance(double d) {
//    if (std::isinf(d)) std::cout << "INF";
//    else               std::cout << d;
//}
//
//// ===== טעינת LGF ל-Adjacency + החזרת מיפוי idx->id =====
//inline Adj loadAdjFromDirectedLGF(const std::string& path,
//    std::vector<int>* out_idx2id = nullptr,
//    bool skipNeg = true)
//{
//    using Digraph = lemon::ListDigraph;
//
//    Digraph g;
//    Digraph::ArcMap<double> w(g);
//    Digraph::NodeMap<int>   nid(g, -1);
//
//    lemon::digraphReader(g, path).nodeMap("id", nid).arcMap("weight", w).run();
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
//    if (out_idx2id) *out_idx2id = ids;
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
//// ----- עזר: BFS לספירת reachable (מתעלם ממשקלים) -----
//inline std::size_t reachable_count(const Adj& adj, int s) {
//    std::vector<char> vis(adj.size(), 0);
//    std::vector<int> q;
//    q.reserve(1024);
//    q.push_back(s);
//    vis[s] = 1;
//
//    for (std::size_t i = 0; i < q.size(); ++i) {
//        int u = q[i];
//        for (const auto& e : adj[u]) {
//            int v = e.first;
//            if (!vis[v]) { vis[v] = 1; q.push_back(v); }
//        }
//    }
//    return q.size();
//}
//
//// ----- עזר: K הצמתים עם דרגת היציאה הגבוהה -----
//inline std::vector<int> top_k_by_outdeg(const Adj& adj, std::size_t K = 10) {
//    if (K == 0) K = 1;
//    if (K > adj.size()) K = adj.size();
//
//    std::vector<std::pair<std::size_t, int>> best;
//    best.reserve(K);
//
//    auto push_candidate = [&](std::size_t deg, int idx) {
//        if (best.size() < K) best.emplace_back(deg, idx);
//        else {
//            std::size_t min_pos = 0;
//            for (std::size_t i = 1; i < best.size(); ++i)
//                if (best[i].first < best[min_pos].first) min_pos = i;
//            if (deg > best[min_pos].first) best[min_pos] = { deg, idx };
//        }
//        };
//
//    for (std::size_t i = 0; i < adj.size(); ++i) push_candidate(adj[i].size(), static_cast<int>(i));
//
//    std::sort(best.begin(), best.end(), [](const auto& a, const auto& b) {
//        if (a.first != b.first) return a.first > b.first;
//        return a.second < b.second;
//        });
//
//    std::vector<int> res;
//    res.reserve(best.size());
//    for (const auto& p : best) res.push_back(p.second);
//    return res;
//}
//
//// ----- בחירת מקור לפי reachability -----
//inline int choose_source_by_reachability(const Adj& adj,
//    double min_ratio = 0.10,
//    std::size_t K = 10,
//    std::size_t* out_reach = nullptr,
//    std::size_t* out_outdeg = nullptr) {
//    const std::size_t n = adj.size();
//    auto candidates = top_k_by_outdeg(adj, K);
//
//    std::size_t best_reach = 0;
//    int best_idx = candidates.empty() ? 0 : candidates[0];
//    std::size_t best_outdeg = candidates.empty() ? 0 : adj[best_idx].size();
//
//    for (int s : candidates) {
//        std::size_t r = reachable_count(adj, s);
//        if (r > best_reach) {
//            best_reach = r;
//            best_idx = s;
//            best_outdeg = adj[s].size();
//        }
//        if (static_cast<double>(r) / static_cast<double>(n) >= min_ratio) break;
//    }
//
//    if (out_reach)  *out_reach = best_reach;
//    if (out_outdeg) *out_outdeg = best_outdeg;
//    return best_idx;
//}
//
//// ===== פרמטרים לפי המאמר =====
//struct PaperParams {
//    int l, k_paper, t;
//    std::size_t M;
//};
//inline PaperParams choose_params_from_paper(std::size_t n) {
//    if (n < 2) return { 1,1,1,1 };
//    const double ln_n = std::log(static_cast<double>(n));
//    int k = (int)std::floor(std::cbrt(ln_n));        if (k < 1) k = 1;
//    int t = (int)std::floor(std::pow(ln_n, 2.0 / 3.0)); if (t < 1) t = 1;
//    int l = (int)std::ceil(ln_n / (double)t);         if (l < 1) l = 1;
//    int exp_int = (l - 1) * t; if (exp_int < 0) exp_int = 0;
//    std::size_t M = (std::size_t)std::pow(2.0, (double)exp_int);
//    if (M == 0) M = 1;
//    return { l, k, t, M };
//}
//
//// ===== LEMON build & helpers (ללא העתקות של טיפוסים לא-ניתנים להעתקה) =====
//using Digraph = lemon::ListDigraph;
//
//// בונה את הגרף מ-LGF, ממלא nid/w, מוצא מקור, ומחזיר את מספר ה-IDים הייחודיים (למיפוי עקבי)
//static void build_lemon_from_lgf(
//    const std::string& lgf_path,
//    int source_id,
//    Digraph& g,
//    Digraph::ArcMap<double>& w,
//    Digraph::NodeMap<int>& nid,
//    Digraph::Node& src,
//    std::size_t& out_n_ids)
//{
//    lemon::digraphReader(g, lgf_path)
//        .nodeMap("id", nid)
//        .arcMap("weight", w)
//        .run();
//
//    // מציאת הצומת מקור
//    src = lemon::INVALID;
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
//        if (nid[n] == source_id) { src = n; break; }
//    }
//
//    // חישוב מספר IDים ייחודיים (לגודל הווקטור)
//    std::vector<int> ids;
//    ids.reserve(lemon::countNodes(g));
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
//    std::sort(ids.begin(), ids.end());
//    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
//    out_n_ids = ids.size();
//}
//
//// מוציא וקטור מרחקים לפי סדר IDים עולה (כמו שבנינו ב-BMSSP)
//static std::vector<double> extract_dist_vector(
//    const Digraph& g,
//    const Digraph::NodeMap<int>& nid,
//    const lemon::Dijkstra<Digraph, Digraph::ArcMap<double>>& dij)
//{
//    std::vector<int> ids;
//    ids.reserve(lemon::countNodes(g));
//    for (lemon::ListDigraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
//    std::sort(ids.begin(), ids.end());
//    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
//
//    std::unordered_map<int, int> id2idx;
//    id2idx.reserve(ids.size());
//    for (int i = 0; i < (int)ids.size(); ++i) id2idx[ids[i]] = i;
//
//    std::vector<double> dist(ids.size(), INF);
//    for (lemon::ListDigraph::NodeIt n(g); n != lemon::INVALID; ++n) {
//        int id = nid[n];
//        auto it = id2idx.find(id);
//        if (it == id2idx.end()) continue;
//        std::size_t idx = (std::size_t)it->second;
//        dist[idx] = dij.reached(n) ? dij.dist(n) : INF;
//    }
//    return dist;
//}
//
//int main() {
//    try {
//        // === קלט — קובץ RB ===
//        const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\patents_main.rb";
//        std::cout << "Input RB file : " << rb_path << "\n";
//
//        // === שלב A: Preprocessing משותף: קריאת RB, סטטיסטיקות, שמירת LGF/GraphML ===
//        auto t_common_start = clk::now();
//
//        rbconv::RBToLemonConverter conv;
//        if (!conv.readRutherfordBoeing(rb_path)) {
//            std::cerr << "Failed to read RB file.\n"; return 1;
//        }
//        conv.printGraphStats();
//
//        const std::size_t dotPos = rb_path.find_last_of('.');
//        const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
//        const std::string lgf_path = base + "_graph.lgf";
//        const std::string graphml_path = base + "_graph.graphml";
//
//        conv.saveToLemonFormat(lgf_path);
//        conv.saveToGraphML(graphml_path);
//
//        auto t_common_end = clk::now();
//        std::chrono::duration<double> time_common = t_common_end - t_common_start;
//
//        // === שלב B: בניית adjacency (הכנה של BMSSP) + בחירת מקור ופרמטרים ===
//        auto t_adj_build_start = clk::now();
//        std::vector<int> idx2id;
//        Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
//        auto t_adj_build_end = clk::now();
//        std::chrono::duration<double> time_bm_build = t_adj_build_end - t_adj_build_start;
//
//        const std::size_t n = adj.size();
//        if (n == 0) { std::cerr << "Adjacency is empty.\n"; return 1; }
//
//        std::size_t reach_est = 0, src_outdeg = 0;
//        int source_idx = choose_source_by_reachability(adj, /*min_ratio=*/0.10, /*K=*/10, &reach_est, &src_outdeg);
//        int source_id = idx2id[source_idx];
//
//        PaperParams prm = choose_params_from_paper(n);
//        const int l = prm.l;
//        const int k_paper = prm.k_paper;
//        const std::size_t M = prm.M;
//        const std::size_t K_work = std::max<std::size_t>(M, 1u << 16);
//
//        // === שלב C: ריצה נטו של BMSSP ===
//        std::vector<double> db(n, INF);
//        db[(std::size_t)source_idx] = 0.0;
//        std::vector<Key> S = { source_idx };
//        auto index_of = [](Key k) { return (std::size_t)k; };
//        double B = INF;
//
//        auto t_bm_run_start = clk::now();
//        auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K_work);
//        auto t_bm_run_end = clk::now();
//        std::chrono::duration<double> time_bm_run = t_bm_run_end - t_bm_run_start;
//
//        // סטטיסטיקות BMSSP
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
//        // === שלב D: בנייה חד-פעמית של גרף LEMON (הכנה של דייקסטרה) ===
//        auto t_dij_build_start = clk::now();
//        Digraph g;
//        Digraph::ArcMap<double> w(g);
//        Digraph::NodeMap<int>   nid(g, -1);
//        Digraph::Node           src;
//        std::size_t n_ids = 0;
//        build_lemon_from_lgf(lgf_path, source_id, g, w, nid, src, n_ids);
//        auto t_dij_build_end = clk::now();
//        std::chrono::duration<double> time_dij_build = t_dij_build_end - t_dij_build_start;
//
//        if (src == lemon::INVALID) {
//            std::cerr << "Dijkstra: source id " << source_id << " not found in LGF.\n";
//            return 1;
//        }
//
//        // === שלב E: ריצה נטו של דייקסטרה ===
//        lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
//        auto t_dij_run_start = clk::now();
//        dij.run(src);
//        auto t_dij_run_end = clk::now();
//        std::chrono::duration<double> time_dij_run = t_dij_run_end - t_dij_run_start;
//
//        // וקטור מרחקים של דייקסטרה (לפי סדר IDים עולה)
//        std::vector<double> d_ref = extract_dist_vector(g, nid, dij);
//        const std::size_t cmpN = std::min<std::size_t>(d_ref.size(), n);
//
//        // === השוואה BMSSP מול Dijkstra ===
//        std::size_t mismatches = 0;
//        double max_abs_diff = 0.0;
//        const double EPS = 1e-9;
//        for (std::size_t i = 0; i < cmpN; ++i) {
//            bool a_inf = std::isinf(db[i]);
//            bool b_inf = std::isinf(d_ref[i]);
//            if (a_inf != b_inf) ++mismatches;
//            else if (!a_inf) {
//                double diff = std::fabs(db[i] - d_ref[i]);
//                if (diff > EPS) ++mismatches;
//                if (diff > max_abs_diff) max_abs_diff = diff;
//            }
//        }
//
//        // === הדפסות מסכמות (כמו בפלט שלך) ===
//        std::cout << "Nodes: " << n << "\n";
//        std::cout << "Chosen source (index): " << source_idx
//            << " | (LGF id): " << source_id
//            << " | outdeg: " << src_outdeg
//            << " | reachable_est: " << reach_est
//            << " (" << (100.0 * (double)reach_est / (double)n) << "%)\n";
//        std::cout << "Params (paper): n=" << n
//            << " | k_paper=" << k_paper
//            << " | t=" << prm.t
//            << " | l=" << l
//            << " | M=" << M << "\n";
//        std::cout << "Exec params  : K_work=" << K_work << " (passed as K to bmssp)\n";
//
//        std::cout << "\n===== Timing (shared preprocessing) =====\n";
//        std::cout << "RB->LGF+GraphML (shared): " << time_common.count() << " s\n";
//
//        std::cout << "\n===== BMSSP timing =====\n";
//        std::cout << "BMSSP build (Adj from LGF): " << time_bm_build.count() << " s\n";
//        std::cout << "BMSSP run (net):           " << time_bm_run.count() << " s\n";
//        std::cout << "BMSSP total (build+run):   " << (time_bm_build + time_bm_run).count() << " s\n";
//
//        std::cout << "\n===== Dijkstra timing =====\n";
//        std::cout << "LEMON build (parse LGF):   " << time_dij_build.count() << " s\n";
//        std::cout << "Dijkstra run (net):        " << time_dij_run.count() << " s\n";
//        std::cout << "Dijkstra total (build+run):" << (time_dij_build + time_dij_run).count() << " s\n";
//
//        std::cout << "\n===== BMSSP summary =====\n";
//        std::cout << "Reachable      : " << reachable
//            << " (" << (100.0 * (double)reachable / (double)n) << "%)\n";
//        std::cout << "Min positive d : "; print_distance((min_finite == INF) ? 0 : min_finite); std::cout << "\n";
//        std::cout << "Max finite d   : "; print_distance(max_finite); std::cout << "\n";
//        std::cout << "B'             : " << out.Bprime << "\n";
//        std::cout << "LGF used       : " << lgf_path << "\n";
//
//        std::cout << "\n===== Comparison (BMSSP vs Dijkstra) =====\n";
//        std::cout << "Compared nodes : " << cmpN << "\n";
//        std::cout << "Mismatches     : " << mismatches << "\n";
//        std::cout << "Max Abs Diff   : " << max_abs_diff << "\n";
//        std::cout << "==========================================\n";
//
//        // הדפסת 20 ראשונים (לא חובה)
//        const std::size_t show = std::min<std::size_t>(n, 20);
//        for (std::size_t i = 0; i < show; ++i) {
//            std::cout << "d[" << i << "] = "; print_distance(db[i]);
//            std::cout << " | d_ref[" << i << "] = "; print_distance((i < d_ref.size() ? d_ref[i] : INF));
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
//
//
//









// main.cpp — BMSSP vs Dijkstra: fair timing (build vs run), logging only
#include <string>
#include <vector>
#include <utility>
#include <limits>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <sstream>

#include "logger.hpp"
#include "rb_to_lemon.hpp"
#include "sssp/algorithms/bmssp.hpp"

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/dijkstra.h>

using clk = std::chrono::steady_clock;

using Key = int;
using Adj = std::vector<std::vector<std::pair<Key, double>>>;
static const double INF = std::numeric_limits<double>::infinity();

// ===== המרת מרחק למחרוזת (במקום להדפיס) =====
static inline std::string distance_to_str(double d) {
    if (std::isinf(d)) return "INF";
    std::ostringstream oss; oss << d; return oss.str();
}

// ===== טעינת LGF ל-Adjacency + החזרת מיפוי idx->id =====
inline Adj loadAdjFromDirectedLGF(const std::string& path,
    std::vector<int>* out_idx2id = nullptr,
    bool skipNeg = true)
{
    using Digraph = lemon::ListDigraph;

    Digraph g;
    Digraph::ArcMap<double> w(g);
    Digraph::NodeMap<int>   nid(g, -1);

    lemon::digraphReader(g, path).nodeMap("id", nid).arcMap("weight", w).run();

    std::vector<int> ids;
    ids.reserve(lemon::countNodes(g));
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    std::unordered_map<int, int> id2idx;
    id2idx.reserve(ids.size());
    for (int i = 0; i < (int)ids.size(); ++i) id2idx[ids[i]] = i;

    if (out_idx2id) *out_idx2id = ids;

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

// ----- עזר: BFS לספירת reachable (מתעלם ממשקלים) -----
inline std::size_t reachable_count(const Adj& adj, int s) {
    std::vector<char> vis(adj.size(), 0);
    std::vector<int> q;
    q.reserve(1024);
    q.push_back(s);
    vis[s] = 1;

    for (std::size_t i = 0; i < q.size(); ++i) {
        int u = q[i];
        for (const auto& e : adj[u]) {
            int v = e.first;
            if (!vis[v]) { vis[v] = 1; q.push_back(v); }
        }
    }
    return q.size();
}

// ----- עזר: K הצמתים עם דרגת היציאה הגבוהה -----
inline std::vector<int> top_k_by_outdeg(const Adj& adj, std::size_t K = 10) {
    if (K == 0) K = 1;
    if (K > adj.size()) K = adj.size();

    std::vector<std::pair<std::size_t, int>> best;
    best.reserve(K);

    auto push_candidate = [&](std::size_t deg, int idx) {
        if (best.size() < K) best.emplace_back(deg, idx);
        else {
            std::size_t min_pos = 0;
            for (std::size_t i = 1; i < best.size(); ++i)
                if (best[i].first < best[min_pos].first) min_pos = i;
            if (deg > best[min_pos].first) best[min_pos] = { deg, idx };
        }
        };

    for (std::size_t i = 0; i < adj.size(); ++i) push_candidate(adj[i].size(), static_cast<int>(i));

    std::sort(best.begin(), best.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first) return a.first > b.first;
        return a.second < b.second;
        });

    std::vector<int> res;
    res.reserve(best.size());
    for (const auto& p : best) res.push_back(p.second);
    return res;
}

// ----- בחירת מקור לפי reachability -----
inline int choose_source_by_reachability(const Adj& adj,
    double min_ratio = 0.10,
    std::size_t K = 10,
    std::size_t* out_reach = nullptr,
    std::size_t* out_outdeg = nullptr) {
    const std::size_t n = adj.size();
    auto candidates = top_k_by_outdeg(adj, K);

    std::size_t best_reach = 0;
    int best_idx = candidates.empty() ? 0 : candidates[0];
    std::size_t best_outdeg = candidates.empty() ? 0 : adj[best_idx].size();

    for (int s : candidates) {
        std::size_t r = reachable_count(adj, s);
        if (r > best_reach) {
            best_reach = r;
            best_idx = s;
            best_outdeg = adj[s].size();
        }
        if (static_cast<double>(r) / static_cast<double>(n) >= min_ratio) break;
    }

    if (out_reach)  *out_reach = best_reach;
    if (out_outdeg) *out_outdeg = best_outdeg;
    return best_idx;
}

// ===== פרמטרים לפי המאמר =====
struct PaperParams {
    int l, k_paper, t;
    std::size_t M;
};
inline PaperParams choose_params_from_paper(std::size_t n) {
    if (n < 2) return { 1,1,1,1 };
    const double ln_n = std::log(static_cast<double>(n));
    int k = (int)std::floor(std::cbrt(ln_n));        if (k < 1) k = 1;
    int t = (int)std::floor(std::pow(ln_n, 2.0 / 3.0)); if (t < 1) t = 1;
    int l = (int)std::ceil(ln_n / (double)t);         if (l < 1) l = 1;
    int exp_int = (l - 1) * t; if (exp_int < 0) exp_int = 0;
    std::size_t M = (std::size_t)std::pow(2.0, (double)exp_int);
    if (M == 0) M = 1;
    return { l, k, t, M };
}

// ===== LEMON build & helpers (ללא העתקות של טיפוסים לא-ניתנים להעתקה) =====
using Digraph = lemon::ListDigraph;

// בונה את הגרף מ-LGF, ממלא nid/w, מוצא מקור, ומחזיר את מספר ה-IDים הייחודיים (למיפוי עקבי)
static void build_lemon_from_lgf(
    const std::string& lgf_path,
    int source_id,
    Digraph& g,
    Digraph::ArcMap<double>& w,
    Digraph::NodeMap<int>& nid,
    Digraph::Node& src,
    std::size_t& out_n_ids)
{
    lemon::digraphReader(g, lgf_path)
        .nodeMap("id", nid)
        .arcMap("weight", w)
        .run();

    // מציאת הצומת מקור
    src = lemon::INVALID;
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
        if (nid[n] == source_id) { src = n; break; }
    }

    // חישוב מספר IDים ייחודיים (לגודל הווקטור)
    std::vector<int> ids;
    ids.reserve(lemon::countNodes(g));
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    out_n_ids = ids.size();
}

// מוציא וקטור מרחקים לפי סדר IDים עולה (כמו שבנינו ב-BMSSP)
static std::vector<double> extract_dist_vector(
    const Digraph& g,
    const Digraph::NodeMap<int>& nid,
    const lemon::Dijkstra<Digraph, Digraph::ArcMap<double>>& dij)
{
    std::vector<int> ids;
    ids.reserve(lemon::countNodes(g));
    for (lemon::ListDigraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    std::unordered_map<int, int> id2idx;
    id2idx.reserve(ids.size());
    for (int i = 0; i < (int)ids.size(); ++i) id2idx[ids[i]] = i;

    std::vector<double> dist(ids.size(), INF);
    for (lemon::ListDigraph::NodeIt n(g); n != lemon::INVALID; ++n) {
        int id = nid[n];
        auto it = id2idx.find(id);
        if (it == id2idx.end()) continue;
        std::size_t idx = (std::size_t)it->second;
        dist[idx] = dij.reached(n) ? dij.dist(n) : INF;
    }
    return dist;
}

int main() {
    try {
        // אפשר לכבות DEBUG אם תרצי רק INFO/ERROR:
        // logsys::current_level() = logsys::Level::INFO;

        // === קלט — קובץ RB ===
        const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\patents_main.rb";
        LOG_INFO() << "Input RB file : " << rb_path;

        // === שלב A: Preprocessing משותף: קריאת RB, סטטיסטיקות, שמירת LGF/GraphML ===
        auto t_common_start = clk::now();

        rbconv::RBToLemonConverter conv;
        if (!conv.readRutherfordBoeing(rb_path)) {
            LOG_ERROR() << "Failed to read RB file.";
            return 1;
        }
        // conv.printGraphStats(); // מבטל הדפסות ישירות

        const std::size_t dotPos = rb_path.find_last_of('.');
        const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
        const std::string lgf_path = base + "_graph.lgf";
        const std::string graphml_path = base + "_graph.graphml";

        conv.saveToLemonFormat(lgf_path);
        conv.saveToGraphML(graphml_path);

        auto t_common_end = clk::now();
        std::chrono::duration<double> time_common = t_common_end - t_common_start;

        // === שלב B: בניית adjacency (הכנה של BMSSP) + בחירת מקור ופרמטרים ===
        auto t_adj_build_start = clk::now();
        std::vector<int> idx2id;
        Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
        auto t_adj_build_end = clk::now();
        std::chrono::duration<double> time_bm_build = t_adj_build_end - t_adj_build_start;

        const std::size_t n = adj.size();
        if (n == 0) { LOG_ERROR() << "Adjacency is empty."; return 1; }

        std::size_t reach_est = 0, src_outdeg = 0;
        int source_idx = choose_source_by_reachability(adj, /*min_ratio=*/0.10, /*K=*/10, &reach_est, &src_outdeg);
        int source_id = idx2id[source_idx];

        PaperParams prm = choose_params_from_paper(n);
        const int l = prm.l;
        const int k_paper = prm.k_paper;
        const std::size_t M = prm.M;
        const std::size_t K_work = std::max<std::size_t>(M, 1u << 16);

        // === שלב C: ריצה נטו של BMSSP ===
        std::vector<double> db(n, INF);
        db[(std::size_t)source_idx] = 0.0;
        std::vector<Key> S = { source_idx };
        auto index_of = [](Key k) { return (std::size_t)k; };
        double B = INF;

        auto t_bm_run_start = clk::now();
        auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K_work);
        auto t_bm_run_end = clk::now();
        std::chrono::duration<double> time_bm_run = t_bm_run_end - t_bm_run_start;

        // סטטיסטיקות BMSSP
        std::size_t reachable = 0;
        double min_finite = INF, max_finite = 0.0;
        for (double d : db) {
            if (!std::isinf(d)) {
                ++reachable;
                if (d > 0.0) min_finite = std::min(min_finite, d);
                max_finite = std::max(max_finite, d);
            }
        }

        // === שלב D: בנייה חד-פעמית של גרף LEMON (הכנה של דייקסטרה) ===
        auto t_dij_build_start = clk::now();
        Digraph g;
        Digraph::ArcMap<double> w(g);
        Digraph::NodeMap<int>   nid(g, -1);
        Digraph::Node           src;
        std::size_t n_ids = 0;
        build_lemon_from_lgf(lgf_path, source_id, g, w, nid, src, n_ids);
        auto t_dij_build_end = clk::now();
        std::chrono::duration<double> time_dij_build = t_dij_build_end - t_dij_build_start;

        if (src == lemon::INVALID) {
            LOG_ERROR() << "Dijkstra: source id " << source_id << " not found in LGF.";
            return 1;
        }

        // === שלב E: ריצה נטו של דייקסטרה ===
        lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
        auto t_dij_run_start = clk::now();
        dij.run(src);
        auto t_dij_run_end = clk::now();
        std::chrono::duration<double> time_dij_run = t_dij_run_end - t_dij_run_start;

        // וקטור מרחקים של דייקסטרה (לפי סדר IDים עולה)
        std::vector<double> d_ref = extract_dist_vector(g, nid, dij);
        const std::size_t cmpN = std::min<std::size_t>(d_ref.size(), n);

        // === השוואה BMSSP מול Dijkstra ===
        std::size_t mismatches = 0;
        double max_abs_diff = 0.0;
        const double EPS = 1e-9;
        for (std::size_t i = 0; i < cmpN; ++i) {
            bool a_inf = std::isinf(db[i]);
            bool b_inf = std::isinf(d_ref[i]);
            if (a_inf != b_inf) ++mismatches;
            else if (!a_inf) {
                double diff = std::fabs(db[i] - d_ref[i]);
                if (diff > EPS) ++mismatches;
                if (diff > max_abs_diff) max_abs_diff = diff;
            }
        }

        // === לוגים מסכמים ===
        LOG_INFO() << "Nodes: " << n;
        LOG_INFO() << "Chosen source (index): " << source_idx
            << " | (LGF id): " << source_id
            << " | outdeg: " << src_outdeg
            << " | reachable_est: " << reach_est
            << " (" << (100.0 * (double)reach_est / (double)n) << "%)";
        LOG_DEBUG() << "Params (paper): n=" << n
            << " | k_paper=" << k_paper
            << " | t=" << prm.t
            << " | l=" << l
            << " | M=" << M;
        LOG_DEBUG() << "Exec params  : K_work=" << K_work << " (passed as K to bmssp)";

        LOG_INFO() << "===== Timing (shared preprocessing) =====";
        LOG_INFO() << "RB->LGF+GraphML (shared): " << time_common.count() << " s";

        LOG_INFO() << "===== BMSSP timing =====";
        LOG_INFO() << "BMSSP build (Adj from LGF): " << time_bm_build.count() << " s";
        LOG_INFO() << "BMSSP run (net):           " << time_bm_run.count() << " s";
        LOG_INFO() << "BMSSP total (build+run):   " << (time_bm_build + time_bm_run).count() << " s";

        LOG_INFO() << "===== Dijkstra timing =====";
        LOG_INFO() << "LEMON build (parse LGF):   " << time_dij_build.count() << " s";
        LOG_INFO() << "Dijkstra run (net):        " << time_dij_run.count() << " s";
        LOG_INFO() << "Dijkstra total (build+run):" << (time_dij_build + time_dij_run).count() << " s";

        LOG_INFO() << "===== BMSSP summary =====";
        LOG_INFO() << "Reachable      : " << reachable
            << " (" << (100.0 * (double)reachable / (double)n) << "%)";
        LOG_INFO() << "Min positive d : " << distance_to_str((min_finite == INF) ? 0 : min_finite);
        LOG_INFO() << "Max finite d   : " << distance_to_str(max_finite);
        LOG_DEBUG() << "B'             : " << out.Bprime;
        LOG_INFO() << "LGF used       : " << lgf_path;

        LOG_INFO() << "===== Comparison (BMSSP vs Dijkstra) =====";
        LOG_INFO() << "Compared nodes : " << cmpN;
        LOG_INFO() << "Mismatches     : " << mismatches;
        LOG_INFO() << "Max Abs Diff   : " << max_abs_diff;
        LOG_INFO() << "==========================================";

        // 20 ראשונים (DEBUG בלבד)
        const std::size_t show = std::min<std::size_t>(n, 20);
        for (std::size_t i = 0; i < show; ++i) {
            LOG_DEBUG() << "d[" << i << "] = " << distance_to_str(db[i])
                << " | d_ref[" << i << "] = " << distance_to_str((i < d_ref.size() ? d_ref[i] : INF));
        }

        return 0;
    }
    catch (const std::exception& ex) {
        LOG_ERROR() << "Exception: " << ex.what();
        return 1;
    }
}
