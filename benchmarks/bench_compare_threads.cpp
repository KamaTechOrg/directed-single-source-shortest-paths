//// benchmarks/bench_compare_threads.cpp
//#include "bench_common.hpp"
//#include "sssp/algorithms/bmssp.hpp"
//
//#include <filesystem>
//#include <cmath>
//#include <algorithm>
//#include <fstream>
//#include <random>
//#include <unordered_map>
//#include <iomanip>
//#include <vector>
//#include <cstdlib>   // getenv
//
//#include <lemon/list_graph.h>
//#include <lemon/lgf_reader.h>
//#include <lemon/dijkstra.h>
//
//// ----- ITT helpers -----
//#if HAS_ITT
//#  include <ittnotify.h>
//#  define ITT_RESUME() __itt_resume()
//#  define ITT_PAUSE()  __itt_pause()
//#else
//#  define ITT_RESUME() ((void)0)
//#  define ITT_PAUSE()  ((void)0)
//#endif
//
//// ----- OMP helpers -----
//#if defined(_OPENMP)
//#  include <omp.h>
//#  define HAS_OMP 1
//#else
//#  define HAS_OMP 0
//#endif
//
//using Digraph = lemon::ListDigraph;
//using Clock = std::chrono::steady_clock;
//using Key = int;
//
//// ---------------- CLI helpers ----------------
//static std::string cli_get_arg(int argc, char** argv, const std::string& keyEq) {
//    for (int i = 1; i < argc; ++i) {
//        std::string s = argv[i];
//        if (s.rfind(keyEq, 0) == 0) return s.substr(keyEq.size());
//    }
//    return {};
//}
//
//// ---------------- Sources helpers ------------
//static std::vector<int> load_sources_file(const std::string& path, std::size_t maxN) {
//    std::vector<int> out; out.reserve(maxN);
//    std::ifstream in(path);
//    if (!in) return {};
//    int v;
//    while (in >> v) {
//        out.push_back(v);
//        if (out.size() == maxN) break;
//    }
//    return out;
//}
//
//static void save_sources_file(const std::string& path, const std::vector<int>& srcs) {
//    std::ofstream out(path, std::ios::trunc);
//    for (int v : srcs) out << v << "\n";
//}
//
//static std::vector<int> make_deterministic_sources(const Adj& adj, std::size_t N) {
//    const auto n = adj.size();
//    std::vector<int> sources; sources.reserve(N);
//    auto top = top_k_by_outdeg(adj, std::min<std::size_t>(N, 200));
//    for (auto v : top) {
//        if (sources.size() == N) break;
//        sources.push_back((int)v);
//    }
//    for (std::size_t v = 0; v < n && sources.size() < N; ++v) {
//        if (std::find(sources.begin(), sources.end(), (int)v) == sources.end()) {
//            sources.push_back((int)v);
//        }
//    }
//    return sources;
//}
//
//// ---------------- Graph helpers --------------
//static std::size_t total_edges(const Adj& adj) {
//    std::size_t E = 0;
//    for (const auto& nbrs : adj) E += nbrs.size();
//    return E;
//}
//
//// -------------- Pretty print table -----------
//static void print_header() {
//    std::cout << std::left
//        << std::setw(12) << "Algo"
//        << std::setw(10) << "Threads"
//        << std::setw(12) << "V"
//        << std::setw(12) << "E"
//        << std::setw(8) << "N"
//        << std::setw(14) << "mean[s]"
//        << std::setw(14) << "median[s]"
//        << std::setw(14) << "p95[s]"
//        << std::setw(18) << "ns/edge (mean)"
//        << std::setw(18) << "ns/edge (p95)"
//        << "\n";
//    std::cout << std::string(12 + 10 + 12 + 12 + 8 + 14 + 14 + 14 + 18 + 18, '-') << "\n";
//}
//
//static void print_row(const std::string& name,
//    int threads,
//    std::size_t V,
//    std::size_t E,
//    std::size_t N,
//    const Stats& st) {
//    const double ns_per_edge_mean = (E ? st.mean * 1e9 / (double)E : 0.0);
//    const double ns_per_edge_p95 = (E ? st.p95 * 1e9 / (double)E : 0.0);
//
//    std::cout << std::left
//        << std::setw(12) << name
//        << std::setw(10) << threads
//        << std::setw(12) << V
//        << std::setw(12) << E
//        << std::setw(8) << N
//        << std::setw(14) << st.mean
//        << std::setw(14) << st.median
//        << std::setw(14) << st.p95
//        << std::setw(18) << ns_per_edge_mean
//        << std::setw(18) << ns_per_edge_p95
//        << "\n";
//}
//
//// ---- הרצה של BMSSP עם מספר חוטים מסוים ----
//static Stats run_bmssp_threads(const std::vector<int>& sources,
//    const Adj& adj,
//    std::size_t V,
//    int l,
//    std::size_t M,
//    std::size_t K_work,
//    int threads_wanted)
//{
//    std::vector<double> times; times.reserve(sources.size());
//    std::vector<double> db(V, INF);
//    auto index_of = [](Key k) { return (std::size_t)k; };
//
//#if HAS_OMP
//    int old_threads = omp_get_max_threads();
//    omp_set_num_threads(threads_wanted);
//#endif
//
//    ITT_RESUME();
//    for (int s : sources) {
//        std::fill(db.begin(), db.end(), INF);
//        db[(std::size_t)s] = 0.0;
//        std::vector<Key> S = { s };
//
//        auto t0 = Clock::now();
//        auto out = sssp::bmssp<Key>(l, INF, S, adj, db, index_of, M, K_work);
//        (void)out;
//        auto t1 = Clock::now();
//
//        times.push_back(std::chrono::duration<double>(t1 - t0).count());
//    }
//    ITT_PAUSE();
//
//#if HAS_OMP
//    omp_set_num_threads(old_threads);
//#endif
//
//    return stats_of(times);
//}
//
//int main(int argc, char** argv) {
//#if HAS_ITT
//    __itt_pause();   
//#endif
//    logsys::current_level() = logsys::Level::ERROR;
//
//    // ==== RB path from CLI or env ====
//    auto exists_file = [](const std::filesystem::path& p) {
//        std::error_code ec;
//        return !p.empty() && std::filesystem::exists(p, ec) && !std::filesystem::is_directory(p, ec);
//        };
//
//    std::string rb_path = cli_get_arg(argc, argv, "--rb=");
//    if (!rb_path.empty() && !exists_file(rb_path)) {
//        LOG_ERROR() << "--rb points to missing file: " << rb_path;
//        return 1;
//    }
//    if (rb_path.empty()) {
//        if (const char* env = std::getenv("SSSP_RB_PATH")) {
//            std::filesystem::path p(env);
//            if (exists_file(p)) rb_path = p.string();
//            else {
//                LOG_ERROR() << "SSSP_RB_PATH points to missing file: " << p.string();
//                return 1;
//            }
//        }
//    }
//    if (rb_path.empty()) {
//        LOG_ERROR() << "RB path not provided. Use --rb=<path> or set SSSP_RB_PATH.";
//        return 1;
//    }
//
//    const std::size_t N = (argc > 2 ? std::stoul(argv[2]) : 50);
//
//    // ==== RB -> LGF (once) ====
//    const std::size_t dotPos = rb_path.find_last_of('.');
//    const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
//    const std::string lgf_path = base + "_graph.lgf";
//
//    if (!std::filesystem::exists(lgf_path)) {
//        CoutSilencer quiet;
//        rbconv::RBToLemonConverter conv;
//        if (!conv.readRutherfordBoeing(rb_path)) {
//            LOG_ERROR() << "Failed to read RB file.";
//            return 1;
//        }
//        conv.saveToLemonFormat(lgf_path);
//    }
//
//    // ==== load Adj ====
//    std::vector<int> idx2id;
//    Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
//    if (adj.empty()) { LOG_ERROR() << "Adj empty"; return 1; }
//
//    const std::size_t V = adj.size();
//    const std::size_t E = total_edges(adj);
//
//    // ==== sources (load or regenerate) ====
//    std::string sources_path = cli_get_arg(argc, argv, "--sources=");
//    std::string regen = cli_get_arg(argc, argv, "--regen-sources=");
//    if (sources_path.empty()) sources_path = base + "_sources.txt";
//
//    std::vector<int> sources;
//    bool need_regen = (!regen.empty() && regen != "0");
//    if (!need_regen) sources = load_sources_file(sources_path, N);
//    if (need_regen || sources.size() < N) {
//        sources = make_deterministic_sources(adj, N);
//        for (int v : sources) {
//            if (v < 0 || (std::size_t)v >= adj.size()) {
//                LOG_ERROR() << "Invalid source id " << v << " for this graph.";
//                return 2;
//            }
//        }
//        save_sources_file(sources_path, sources);
//    }
//
//    // ==== build LEMON for dijkstra ====
//    Digraph g;
//    Digraph::ArcMap<double> w(g);
//    Digraph::NodeMap<int>   nid(g, -1);
//    lemon::digraphReader(g, lgf_path)
//        .nodeMap("id", nid)
//        .arcMap("weight", w)
//        .run();
//
//    std::unordered_map<int, Digraph::Node> node_by_id;
//    node_by_id.reserve(lemon::countNodes(g));
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n)
//        node_by_id.emplace(nid[n], n);
//
//    // ==== BMSSP params ====
//    const double ln_n = std::log((double)V);
//    int k = (int)std::floor(std::cbrt(ln_n)); if (k < 1) k = 1;
//    int t = (int)std::floor(std::pow(ln_n, 2.0 / 3.0)); if (t < 1) t = 1;
//    int l = (int)std::ceil(ln_n / (double)t); if (l < 1) l = 1;
//    int exp_int = (l - 1) * t; if (exp_int < 0) exp_int = 0;
//    std::size_t M = (std::size_t)std::pow(2.0, (double)exp_int); if (M == 0) M = 1;
//    const std::size_t K_work = (std::size_t)std::max<std::size_t>(M, 1u << 16);
//
//    // ==== warmups ====
//    {
//        std::vector<double> db(V, INF);
//        if (!sources.empty()) {
//            int s0 = sources[0];
//            db[(std::size_t)s0] = 0.0;
//            std::vector<Key> S0 = { s0 };
//            auto out0 = sssp::bmssp<Key>(l, INF, S0, adj, db,
//                [](Key k) {return (std::size_t)k; },
//                M, K_work);
//            (void)out0;
//        }
//    }
//    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
//    {
//        if (!sources.empty()) {
//            int s0 = sources[0];
//            auto it = node_by_id.find(idx2id[s0]);
//            if (it != node_by_id.end()) {
//                dij.init();
//                dij.addSource(it->second);
//                dij.start();
//            }
//        }
//    }
//
//    // ==== measure Dijkstra ====
//    std::vector<double> times_dijk; times_dijk.reserve(sources.size());
//    {
//#if HAS_ITT
//        __itt_resume();   // מכאן תמדוד את דייקסטרה
//#endif
//        for (int s : sources) {
//            auto it = node_by_id.find(idx2id[s]);
//            if (it == node_by_id.end()) continue;
//            dij.init();
//            dij.addSource(it->second);
//            auto t0 = Clock::now();
//            dij.start();
//            auto t1 = Clock::now();
//            times_dijk.push_back(std::chrono::duration<double>(t1 - t0).count());
//        }
//#if HAS_ITT
//        __itt_pause();    // לסגור אחרי הדייקסטרה
//#endif
//    }
//    const Stats st_dijk = stats_of(times_dijk);
//
//    // ==== measure BMSSP for 1, 4, 12 threads ====
//#if HAS_ITT
//    __itt_resume();   // להדליק שוב למדידת ה-BMSSP
//#endif
//    const Stats st_bmssp_1 = run_bmssp_threads(sources, adj, V, l, M, K_work, 1);
//    const Stats st_bmssp_4 = run_bmssp_threads(sources, adj, V, l, M, K_work, 4);
//    const Stats st_bmssp_12 = run_bmssp_threads(sources, adj, V, l, M, K_work, 12);
//#if HAS_ITT
//    __itt_pause();    // לסגור שוב
//#endif
//
//    // ==== print table ====
//    logsys::current_level() = logsys::Level::INFO;
//    print_header();
//    print_row("Dijkstra", 1, V, E, sources.size(), st_dijk);
//    print_row("BMSSP", 1, V, E, sources.size(), st_bmssp_1);
//    print_row("BMSSP", 4, V, E, sources.size(), st_bmssp_4);
//    print_row("BMSSP", 12, V, E, sources.size(), st_bmssp_12);
//
//    return 0;
//}




// benchmarks/bench_compare_threads.cpp
#include "bench_common.hpp"
#include "sssp/algorithms/bmssp.hpp"

#include <filesystem>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <random>
#include <unordered_map>
#include <iomanip>
#include <vector>
#include <cstdlib>   // getenv
#include <chrono>
#include <iostream>

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/dijkstra.h>

// ----- ITT helpers -----
#if HAS_ITT
#  include <ittnotify.h>
#  define ITT_RESUME() __itt_resume()
#  define ITT_PAUSE()  __itt_pause()
#else
#  define ITT_RESUME() ((void)0)
#  define ITT_PAUSE()  ((void)0)
#endif

// ----- OMP helpers -----
#if HAS_OMP
#include <omp.h>
#endif
#if defined(_OPENMP)
#  include <omp.h>
#  define HAS_OMP 1
#else
#  define HAS_OMP 0
#endif

using Digraph = lemon::ListDigraph;
using Clock = std::chrono::steady_clock;
using Key = int;

// ========= helper for timestamps =========
static void log_ts(const char* msg) {
    using namespace std::chrono;
    auto now = steady_clock::now().time_since_epoch();
    auto ms = duration_cast<milliseconds>(now).count();
    std::cerr << "[t=" << ms << " ms] " << msg << "\n";
}

// ---------------- CLI helpers ----------------
static std::string cli_get_arg(int argc, char** argv, const std::string& keyEq) {
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s.rfind(keyEq, 0) == 0) return s.substr(keyEq.size());
    }
    return {};
}

// ---------------- Sources helpers ------------
static std::vector<int> load_sources_file(const std::string& path, std::size_t maxN) {
    std::vector<int> out; out.reserve(maxN);
    std::ifstream in(path);
    if (!in) return {};
    int v;
    while (in >> v) {
        out.push_back(v);
        if (out.size() == maxN) break;
    }
    return out;
}

static void save_sources_file(const std::string& path, const std::vector<int>& srcs) {
    std::ofstream out(path, std::ios::trunc);
    for (int v : srcs) out << v << "\n";
}

static std::vector<int> make_deterministic_sources(const Adj& adj, std::size_t N) {
    const auto n = adj.size();
    std::vector<int> sources; sources.reserve(N);
    auto top = top_k_by_outdeg(adj, std::min<std::size_t>(N, 200));
    for (auto v : top) {
        if (sources.size() == N) break;
        sources.push_back((int)v);
    }
    for (std::size_t v = 0; v < n && sources.size() < N; ++v) {
        if (std::find(sources.begin(), sources.end(), (int)v) == sources.end()) {
            sources.push_back((int)v);
        }
    }
    return sources;
}

// ---------------- Graph helpers --------------
static std::size_t total_edges(const Adj& adj) {
    std::size_t E = 0;
    for (const auto& nbrs : adj) E += nbrs.size();
    return E;
}

// -------------- Pretty print table -----------
static void print_header() {
    std::cout << std::left
        << std::setw(12) << "Algo"
        << std::setw(10) << "Threads"
        << std::setw(12) << "V"
        << std::setw(12) << "E"
        << std::setw(8) << "N"
        << std::setw(14) << "mean[s]"
        << std::setw(14) << "median[s]"
        << std::setw(14) << "p95[s]"
        << std::setw(18) << "ns/edge (mean)"
        << std::setw(18) << "ns/edge (p95)"
        << "\n";
    std::cout << std::string(12 + 10 + 12 + 12 + 8 + 14 + 14 + 14 + 18 + 18, '-') << "\n";
}

static void print_row(const std::string& name,
    int threads,
    std::size_t V,
    std::size_t E,
    std::size_t N,
    const Stats& st) {
    const double ns_per_edge_mean = (E ? st.mean * 1e9 / (double)E : 0.0);
    const double ns_per_edge_p95 = (E ? st.p95 * 1e9 / (double)E : 0.0);

    std::cout << std::left
        << std::setw(12) << name
        << std::setw(10) << threads
        << std::setw(12) << V
        << std::setw(12) << E
        << std::setw(8) << N
        << std::setw(14) << st.mean
        << std::setw(14) << st.median
        << std::setw(14) << st.p95
        << std::setw(18) << ns_per_edge_mean
        << std::setw(18) << ns_per_edge_p95
        << "\n";
}

// ---- הרצה של BMSSP עם מספר חוטים מסוים ----
static Stats run_bmssp_threads(const std::vector<int>& sources,
    const Adj& adj,
    std::size_t V,
    int l,
    std::size_t M,
    std::size_t K_work,
    int threads_wanted)
{
    std::vector<double> times; times.reserve(sources.size());
    std::vector<double> db(V, INF);
    auto index_of = [](Key k) { return (std::size_t)k; };

#if HAS_OMP
    int old_threads = omp_get_max_threads();
    omp_set_num_threads(threads_wanted);
#endif

    std::cerr << "[BMSSP] start run with " << threads_wanted << " threads, "
        << "sources=" << sources.size() << ", V=" << V
        << ", M=" << M << ", K_work=" << K_work << "\n";

    ITT_RESUME();
    std::size_t src_idx = 0;
    for (int s : sources) {
        if ((src_idx % 10) == 0) {
            std::cerr << "[BMSSP]   source " << src_idx << "/" << sources.size() << "\n";
        }
        ++src_idx;

        std::fill(db.begin(), db.end(), INF);
        db[(std::size_t)s] = 0.0;
        std::vector<Key> S = { s };

        auto t0 = Clock::now();
        auto out = sssp::bmssp<Key>(l, INF, S, adj, db, index_of, M, K_work);
        (void)out;
        auto t1 = Clock::now();

        times.push_back(std::chrono::duration<double>(t1 - t0).count());
    }
    ITT_PAUSE();

#if HAS_OMP
    omp_set_num_threads(old_threads);
#endif

    return stats_of(times);
}

int main(int argc, char** argv) {
#if HAS_ITT
    __itt_pause();
#endif
    logsys::current_level() = logsys::Level::ERROR;
    log_ts("main: start");

    // ==== RB path from CLI or env ====
    auto exists_file = [](const std::filesystem::path& p) {
        std::error_code ec;
        return !p.empty() && std::filesystem::exists(p, ec) && !std::filesystem::is_directory(p, ec);
        };

    std::string rb_path = cli_get_arg(argc, argv, "--rb=");
    if (!rb_path.empty() && !exists_file(rb_path)) {
        LOG_ERROR() << "--rb points to missing file: " << rb_path;
        return 1;
    }
    if (rb_path.empty()) {
        if (const char* env = std::getenv("SSSP_RB_PATH")) {
            std::filesystem::path p(env);
            if (exists_file(p)) rb_path = p.string();
            else {
                LOG_ERROR() << "SSSP_RB_PATH points to missing file: " << p.string();
                return 1;
            }
        }
    }
    if (rb_path.empty()) {
        LOG_ERROR() << "RB path not provided. Use --rb=<path> or set SSSP_RB_PATH.";
        return 1;
    }
    log_ts(("main: RB path = " + rb_path).c_str());

    const std::size_t N = (argc > 2 ? std::stoul(argv[2]) : 50);

    // ==== RB -> LGF (once) ====
    const std::size_t dotPos = rb_path.find_last_of('.');
    const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
    const std::string lgf_path = base + "_graph.lgf";

    if (!std::filesystem::exists(lgf_path)) {
        log_ts("main: LGF not found, converting RB -> LGF ...");
        CoutSilencer quiet;
        rbconv::RBToLemonConverter conv;
        if (!conv.readRutherfordBoeing(rb_path)) {
            LOG_ERROR() << "Failed to read RB file.";
            return 1;
        }
        conv.saveToLemonFormat(lgf_path);
        log_ts("main: conversion done.");
    }
    else {
        log_ts("main: LGF already exists, skipping conversion.");
    }

    // ==== load Adj ====
    log_ts("main: loading Adj from LGF...");
    std::vector<int> idx2id;
    Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
    if (adj.empty()) { LOG_ERROR() << "Adj empty"; return 1; }
    const std::size_t V = adj.size();
    const std::size_t E = total_edges(adj);
    {
        std::ostringstream oss;
        oss << "main: Adj loaded. V=" << V << ", E=" << E;
        log_ts(oss.str().c_str());
    }

    // ==== sources (load or regenerate) ====
    log_ts("main: preparing sources...");
    std::string sources_path = cli_get_arg(argc, argv, "--sources=");
    std::string regen = cli_get_arg(argc, argv, "--regen-sources=");
    if (sources_path.empty()) sources_path = base + "_sources.txt";

    std::vector<int> sources;
    bool need_regen = (!regen.empty() && regen != "0");
    if (!need_regen) sources = load_sources_file(sources_path, N);
    if (need_regen || sources.size() < N) {
        sources = make_deterministic_sources(adj, N);
        for (int v : sources) {
            if (v < 0 || (std::size_t)v >= adj.size()) {
                LOG_ERROR() << "Invalid source id " << v << " for this graph.";
                return 2;
            }
        }
        save_sources_file(sources_path, sources);
    }
    {
        std::ostringstream oss;
        oss << "main: sources ready. count=" << sources.size();
        log_ts(oss.str().c_str());
    }

    // ==== build LEMON for dijkstra ====
    log_ts("main: building LEMON digraph for Dijkstra...");
    Digraph g;
    Digraph::ArcMap<double> w(g);
    Digraph::NodeMap<int>   nid(g, -1);
    lemon::digraphReader(g, lgf_path)
        .nodeMap("id", nid)
        .arcMap("weight", w)
        .run();
    log_ts("main: LEMON digraph ready.");

    std::unordered_map<int, Digraph::Node> node_by_id;
    node_by_id.reserve(lemon::countNodes(g));
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n)
        node_by_id.emplace(nid[n], n);

    // ==== BMSSP params ====
    const double ln_n = std::log((double)V);
    int k = (int)std::floor(std::cbrt(ln_n)); if (k < 1) k = 1;
    int t = (int)std::floor(std::pow(ln_n, 2.0 / 3.0)); if (t < 1) t = 1;
    int l = (int)std::ceil(ln_n / (double)t);          if (l < 1) l = 1;
    int exp_int = (l - 1) * t;                          if (exp_int < 0) exp_int = 0;
    std::size_t M = (std::size_t)std::pow(2.0, (double)exp_int); if (M == 0) M = 1;
    const std::size_t K_work = (std::size_t)std::max<std::size_t>(M, 1u << 16);

    {
        std::ostringstream oss;
        oss << "main: BMSSP params: l=" << l
            << " M=" << M
            << " K_work=" << K_work
            << " (ln_n=" << ln_n << ")";
        log_ts(oss.str().c_str());
    }

    // ==== warmups ====
    log_ts("main: warmup BMSSP on 1 source...");
    {
        std::vector<double> db(V, INF);
        if (!sources.empty()) {
            int s0 = sources[0];
            db[(std::size_t)s0] = 0.0;
            std::vector<Key> S0 = { s0 };
            auto out0 = sssp::bmssp<Key>(l, INF, S0, adj, db,
                [](Key k) {return (std::size_t)k; },
                M, K_work);
            (void)out0;
        }
    }
    log_ts("main: warmup done.");

    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
    {
        log_ts("main: warmup Dijkstra...");
        if (!sources.empty()) {
            int s0 = sources[0];
            auto it = node_by_id.find(idx2id[s0]);
            if (it != node_by_id.end()) {
                dij.init();
                dij.addSource(it->second);
                dij.start();
            }
        }
        log_ts("main: warmup Dijkstra done.");
    }

    // ==== measure Dijkstra ====
    log_ts("main: measure Dijkstra...");
    std::vector<double> times_dijk; times_dijk.reserve(sources.size());
    {
#if HAS_ITT
        __itt_resume();
#endif
        for (int s : sources) {
            auto it = node_by_id.find(idx2id[s]);
            if (it == node_by_id.end()) continue;
            dij.init();
            dij.addSource(it->second);
            auto t0 = Clock::now();
            dij.start();
            auto t1 = Clock::now();
            times_dijk.push_back(std::chrono::duration<double>(t1 - t0).count());
        }
#if HAS_ITT
        __itt_pause();
#endif
    }
    log_ts("main: Dijkstra done.");
    const Stats st_dijk = stats_of(times_dijk);

    // ==== measure BMSSP for 1, 4, 12 threads ====
    log_ts("main: measure BMSSP (1,4,12 threads)...");
#if HAS_ITT
    __itt_resume();
#endif
    const Stats st_bmssp_1 = run_bmssp_threads(sources, adj, V, l, M, K_work, 1);
    const Stats st_bmssp_4 = run_bmssp_threads(sources, adj, V, l, M, K_work, 4);
    const Stats st_bmssp_12 = run_bmssp_threads(sources, adj, V, l, M, K_work, 12);
#if HAS_ITT
    __itt_pause();
#endif
    log_ts("main: BMSSP measurements done.");

    // ==== print table ====
    logsys::current_level() = logsys::Level::INFO;
    print_header();
    print_row("Dijkstra", 1, V, E, sources.size(), st_dijk);
    print_row("BMSSP", 1, V, E, sources.size(), st_bmssp_1);
    print_row("BMSSP", 4, V, E, sources.size(), st_bmssp_4);
    print_row("BMSSP", 12, V, E, sources.size(), st_bmssp_12);

    log_ts("main: done.");
    return 0;
}
