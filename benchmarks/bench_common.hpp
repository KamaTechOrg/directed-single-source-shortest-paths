#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "logger.hpp"         
#include "rb_to_lemon.hpp"        
#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/dijkstra.h>

using clk = std::chrono::steady_clock;
using Key = int;
using Adj = std::vector<std::vector<std::pair<Key, double>>>;
static const double INF = std::numeric_limits<double>::infinity();
using Digraph = lemon::ListDigraph;

// ---- משתיק הדפסות std::cout/err של ספריות צד ----
struct CoutSilencer {
    std::streambuf* old_cout = nullptr;
    std::streambuf* old_cerr = nullptr;
    std::ofstream   nullfile;
    CoutSilencer() {
#ifdef _WIN32
        nullfile.open("NUL");
#else
        nullfile.open("/dev/null");
#endif
        old_cout = std::cout.rdbuf(nullfile.rdbuf());
        old_cerr = std::cerr.rdbuf(nullfile.rdbuf());
    }
    ~CoutSilencer() {
        std::cout.rdbuf(old_cout);
        std::cerr.rdbuf(old_cerr);
    }
};

// ---- עזר: טעינת LGF => Adj + idx2id ----
inline Adj loadAdjFromDirectedLGF(const std::string& path,
    std::vector<int>* out_idx2id = nullptr,
    bool skipNeg = true)
{
    Digraph g;
    Digraph::ArcMap<double> w(g);
    Digraph::NodeMap<int>   nid(g, -1);
    lemon::digraphReader(g, path).nodeMap("id", nid).arcMap("weight", w).run();

    std::vector<int> ids; ids.reserve(lemon::countNodes(g));
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    std::unordered_map<int, int> id2idx; id2idx.reserve(ids.size());
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

// ---- עזר: K בעלי דרגת יציאה גבוהה ----
inline std::vector<int> top_k_by_outdeg(const Adj& adj, std::size_t K = 100) {
    if (K == 0) K = 1; if (K > adj.size()) K = adj.size();
    std::vector<std::pair<std::size_t, int>> best; best.reserve(K);
    auto push = [&](std::size_t deg, int idx) {
        if (best.size() < K) best.emplace_back(deg, idx);
        else {
            std::size_t m = 0; for (std::size_t i = 1; i < best.size(); ++i)
                if (best[i].first < best[m].first) m = i;
            if (deg > best[m].first) best[m] = { deg, idx };
        }
        };
    for (std::size_t i = 0; i < adj.size(); ++i) push(adj[i].size(), (int)i);
    std::sort(best.begin(), best.end(), [](auto& a, auto& b) {
        if (a.first != b.first) return a.first > b.first; return a.second < b.second; });
    std::vector<int> res; res.reserve(best.size());
    for (auto& p : best) res.push_back(p.second);
    return res;
}

// ---- בחירת סט מקורות: חלק top-K, השאר רנדומלי ----
inline std::vector<int> make_source_set(const Adj& adj,
    std::size_t N,
    const std::vector<int>& preferTop = {})
{
    std::vector<int> srcs; srcs.reserve(N);
    for (int s : preferTop) { if (srcs.size() == N) break; srcs.push_back(s); }
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(0, (int)adj.size() - 1);
    std::unordered_set<int> used(srcs.begin(), srcs.end());
    while (srcs.size() < N) {
        int s = dist(rng);
        if (used.insert(s).second) srcs.push_back(s);
    }
    return srcs;
}

// ---- בניית גרף LEMON מה-LGF + מציאת source Node לפי id ----
inline void build_lemon_from_lgf(const std::string& lgf_path,
    Digraph& g,
    Digraph::ArcMap<double>& w,
    Digraph::NodeMap<int>& nid)
{
    lemon::digraphReader(g, lgf_path).nodeMap("id", nid).arcMap("weight", w).run();
}

inline std::unordered_map<int, Digraph::Node>
build_node_by_id(const Digraph& g, const Digraph::NodeMap<int>& nid) {
    std::unordered_map<int, Digraph::Node> mp;
    mp.reserve(lemon::countNodes(g));
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) mp.emplace(nid[n], n);
    return mp;
}

// ---- סטטיסטיקות ----
struct Stats { double mean, median, p95; };
inline Stats stats_of(std::vector<double> v) {
    if (v.empty()) return { 0,0,0 };
    double sum = 0; for (double x : v) sum += x;
    double mean = sum / v.size();
    auto mid = v.begin() + v.size() / 2;
    std::nth_element(v.begin(), mid, v.end());
    double med = *mid;
    std::size_t p95i = (std::size_t)std::floor(0.95 * (v.size() - 1));
    std::nth_element(v.begin(), v.begin() + p95i, v.end());
    double p95 = v[p95i];
    return { mean, med, p95 };
}
