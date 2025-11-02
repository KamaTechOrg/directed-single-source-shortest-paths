//// main.cpp — BMSSP vs Dijkstra: fair timing (build vs run), logging only
//#include <string>
//#include <vector>
//#include <utility>
//#include <limits>
//#include <chrono>
//#include <cmath>
//#include <algorithm>
//#include <unordered_map>
//#include <sstream>
//
//#include "logger.hpp"
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
//// ===== המרת מרחק למחרוזת (במקום להדפיס) =====
//static inline std::string distance_to_str(double d) {
//    if (std::isinf(d)) return "INF";
//    std::ostringstream oss; oss << d; return oss.str();
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
//        // אפשר לכבות DEBUG אם תרצי רק INFO/ERROR:
//        // logsys::current_level() = logsys::Level::INFO;
//
//        // === קלט — קובץ RB ===
//        const std::string rb_path = "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\patents_main.rb";
//        LOG_INFO() << "Input RB file : " << rb_path;
//
//        // === שלב A: Preprocessing משותף: קריאת RB, סטטיסטיקות, שמירת LGF/GraphML ===
//        auto t_common_start = clk::now();
//
//        rbconv::RBToLemonConverter conv;
//        if (!conv.readRutherfordBoeing(rb_path)) {
//            LOG_ERROR() << "Failed to read RB file.";
//            return 1;
//        }
//        // conv.printGraphStats(); // מבטל הדפסות ישירות
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
//        if (n == 0) { LOG_ERROR() << "Adjacency is empty."; return 1; }
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
//            LOG_ERROR() << "Dijkstra: source id " << source_id << " not found in LGF.";
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
//        // === לוגים מסכמים ===
//        LOG_INFO() << "Nodes: " << n;
//        LOG_INFO() << "Chosen source (index): " << source_idx
//            << " | (LGF id): " << source_id
//            << " | outdeg: " << src_outdeg
//            << " | reachable_est: " << reach_est
//            << " (" << (100.0 * (double)reach_est / (double)n) << "%)";
//        LOG_DEBUG() << "Params (paper): n=" << n
//            << " | k_paper=" << k_paper
//            << " | t=" << prm.t
//            << " | l=" << l
//            << " | M=" << M;
//        LOG_DEBUG() << "Exec params  : K_work=" << K_work << " (passed as K to bmssp)";
//
//        LOG_INFO() << "===== Timing (shared preprocessing) =====";
//        LOG_INFO() << "RB->LGF+GraphML (shared): " << time_common.count() << " s";
//
//        LOG_INFO() << "===== BMSSP timing =====";
//        LOG_INFO() << "BMSSP build (Adj from LGF): " << time_bm_build.count() << " s";
//        LOG_INFO() << "BMSSP run (net):           " << time_bm_run.count() << " s";
//        LOG_INFO() << "BMSSP total (build+run):   " << (time_bm_build + time_bm_run).count() << " s";
//
//        LOG_INFO() << "===== Dijkstra timing =====";
//        LOG_INFO() << "LEMON build (parse LGF):   " << time_dij_build.count() << " s";
//        LOG_INFO() << "Dijkstra run (net):        " << time_dij_run.count() << " s";
//        LOG_INFO() << "Dijkstra total (build+run):" << (time_dij_build + time_dij_run).count() << " s";
//
//        LOG_INFO() << "===== BMSSP summary =====";
//        LOG_INFO() << "Reachable      : " << reachable
//            << " (" << (100.0 * (double)reachable / (double)n) << "%)";
//        LOG_INFO() << "Min positive d : " << distance_to_str((min_finite == INF) ? 0 : min_finite);
//        LOG_INFO() << "Max finite d   : " << distance_to_str(max_finite);
//        LOG_DEBUG() << "B'             : " << out.Bprime;
//        LOG_INFO() << "LGF used       : " << lgf_path;
//
//        LOG_INFO() << "===== Comparison (BMSSP vs Dijkstra) =====";
//        LOG_INFO() << "Compared nodes : " << cmpN;
//        LOG_INFO() << "Mismatches     : " << mismatches;
//        LOG_INFO() << "Max Abs Diff   : " << max_abs_diff;
//        LOG_INFO() << "==========================================";
//
//        // 20 ראשונים (DEBUG בלבד)
//        const std::size_t show = std::min<std::size_t>(n, 20);
//        for (std::size_t i = 0; i < show; ++i) {
//            LOG_DEBUG() << "d[" << i << "] = " << distance_to_str(db[i])
//                << " | d_ref[" << i << "] = " << distance_to_str((i < d_ref.size() ? d_ref[i] : INF));
//        }
//
//        return 0;
//    }
//    catch (const std::exception& ex) {
//        LOG_ERROR() << "Exception: " << ex.what();
//        return 1;
//    }
//}



//כולל קבלת קלט ניתוב מהמשתמש
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
#include <filesystem>
#include <iostream>

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
    int k = (int)std::floor(std::cbrt(ln_n));           if (k < 1) k = 1;
    int t = (int)std::floor(std::pow(ln_n, 2.0 / 3.0)); if (t < 1) t = 1;
    int l = (int)std::ceil(ln_n / (double)t);           if (l < 1) l = 1;
    int exp_int = (l - 1) * t; if (exp_int < 0) exp_int = 0;
    std::size_t M = (std::size_t)std::pow(2.0, (double)exp_int);
    if (M == 0) M = 1;
    return { l, k, t, M };
}

// ===== LEMON build & helpers =====
using Digraph = lemon::ListDigraph;

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

// ---- קבלת נתיב קלט מהמשתמש (CLI או פרומפט) ----
static std::string get_input_rb_path(int argc, char** argv) {
    if (argc > 1) {
        return std::string(argv[1]);
    }
    std::string p;
    std::cout << "Enter path to RB file: ";
    std::getline(std::cin, p);
    return p;
}

int main(int argc, char** argv) {
    try {
        // logsys::current_level() = logsys::Level::INFO;

        // === קלט — קובץ RB (מהמשתמש) ===
        const std::string rb_path = get_input_rb_path(argc, argv);
        if (rb_path.empty()) {
            LOG_ERROR() << "No RB path provided.";
            return 1;
        }
        if (!std::filesystem::exists(rb_path)) {
            LOG_ERROR() << "RB path does not exist: " << rb_path;
            return 1;
        }
        LOG_INFO() << "Input RB file : " << rb_path;

        // === שלב A: Preprocessing משותף: קריאת RB, סטטיסטיקות, שמירת LGF/GraphML ===
        auto t_common_start = clk::now();

        rbconv::RBToLemonConverter conv;
        if (!conv.readRutherfordBoeing(rb_path)) {
            LOG_ERROR() << "Failed to read RB file.";
            return 1;
        }

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
