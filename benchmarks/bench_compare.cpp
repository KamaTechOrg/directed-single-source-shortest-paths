// benchmarks/bench_compare.cpp
#include "bench_common.hpp"
#include "sssp/algorithms/bmssp.hpp"

#include <filesystem>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <random>
#include <unordered_map>
#include <iomanip>      

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/dijkstra.h>

#include "config_paths.hpp"

// ----- ITT helpers -----
#if HAS_ITT
#include <ittnotify.h>
#define ITT_RESUME() __itt_resume()
#define ITT_PAUSE()  __itt_pause()
#else
#define ITT_RESUME() ((void)0)
#define ITT_PAUSE()  ((void)0)
#endif

using Digraph = lemon::ListDigraph;
using Clock = std::chrono::steady_clock;

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
        << std::setw(10) << "Algo"
        << std::setw(12) << "V"
        << std::setw(12) << "E"
        << std::setw(8) << "N"
        << std::setw(14) << "mean[s]"
        << std::setw(14) << "median[s]"
        << std::setw(14) << "p95[s]"
        << std::setw(18) << "ns/edge (mean)"
        << std::setw(18) << "ns/edge (p95)"
        << "\n";
    std::cout << std::string(10 + 12 + 12 + 8 + 14 + 14 + 14 + 18 + 18, '-') << "\n";
}
static void print_row(const std::string& name, std::size_t V, std::size_t E,
    std::size_t N, const Stats& st)
{
    const double ns_per_edge_mean = (E ? st.mean * 1e9 / (double)E : 0.0);
    const double ns_per_edge_p95 = (E ? st.p95 * 1e9 / (double)E : 0.0);

    std::cout << std::left
        << std::setw(10) << name
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

int main(int argc, char** argv) {
    logsys::current_level() = logsys::Level::ERROR;

    // ==== קלט: RB + N ====
    std::string rb_path = cli_get_arg(argc, argv, "--rb=");
    if (rb_path.empty()) {
        rb_path = std::string(SSSP_DEFAULT_RB);
    }

    const std::size_t N = (argc > 2 ? std::stoul(argv[2]) : 50);

    // ==== המרה RB->LGF (מחוץ למדידה) ====
    const std::size_t dotPos = rb_path.find_last_of('.');
    const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
    const std::string lgf_path = base + "_graph.lgf";
    const std::string graphml_path = base + "_graph.graphml";

    if (!std::filesystem::exists(lgf_path)) {
        CoutSilencer quiet;
        rbconv::RBToLemonConverter conv;
        if (!conv.readRutherfordBoeing(rb_path)) {
            LOG_ERROR() << "Failed to read RB file.";
            return 1;
        }
        conv.saveToLemonFormat(lgf_path);
        // conv.saveToGraphML(graphml_path);
    }
    else {
        LOG_INFO() << "LGF file already exists, skipping conversion.";
    }

    // ==== טוענים Adj פעם אחת (מחוץ למדידה) ====
    std::vector<int> idx2id;
    Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
    if (adj.empty()) { LOG_ERROR() << "Adj empty"; return 1; }

    const std::size_t V = adj.size();
    const std::size_t E = total_edges(adj);

    // ==== בוחרים מקורות דטרמיניסטיים/מנהל קובץ מקורות ====
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

    // ==== בונים גרף LEMON פעם אחת (לדייקסטרה) ====
    Digraph g;
    Digraph::ArcMap<double> w(g);
    Digraph::NodeMap<int>   nid(g, -1);

    lemon::digraphReader(g, lgf_path)
        .nodeMap("id", nid)
        .arcMap("weight", w)
        .run();

    // Map: id -> Node
    std::unordered_map<int, Digraph::Node> node_by_id;
    node_by_id.reserve(lemon::countNodes(g));
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
        node_by_id.emplace(nid[n], n);
    }

    // ==== פרמטרים ל-BMSSP (מהלוגיקה שלך) ====
    const double ln_n = std::log((double)V);
    int k = (int)std::floor(std::cbrt(ln_n)); if (k < 1) k = 1;
    int t = (int)std::floor(std::pow(ln_n, 2.0 / 3.0)); if (t < 1) t = 1;
    int l = (int)std::ceil(ln_n / (double)t); if (l < 1) l = 1;
    int exp_int = (l - 1) * t; if (exp_int < 0) exp_int = 0;
    std::size_t M = (std::size_t)std::pow(2.0, (double)exp_int); if (M == 0) M = 1;
    const std::size_t K_work = std::max<std::size_t>(M, 1u << 16);

    // ==== Warmups (לא נאספים) ====
    // BMSSP
    {
        std::vector<double> db(V, INF);
        if (!sources.empty()) {
            int s0 = sources[0];
            db[(std::size_t)s0] = 0.0;
            std::vector<Key> S0 = { s0 };
            auto t0 = Clock::now();
            auto out0 = sssp::bmssp<Key>(l, INF, S0, adj, db,
                [](Key k) {return (std::size_t)k; },
                M, K_work);
            auto t1 = Clock::now(); (void)out0;
            (void)std::chrono::duration<double>(t1 - t0).count();
        }
    }
    // Dijkstra
    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
    {
        if (!sources.empty()) {
            int s0 = sources[0];
            auto it = node_by_id.find(idx2id[s0]);
            if (it != node_by_id.end()) {
                dij.init();
                dij.addSource(it->second);
                auto t0 = Clock::now();
                dij.start();
                auto t1 = Clock::now(); (void)std::chrono::duration<double>(t1 - t0).count();
            }
        }
    }

    // ==== מדידת BMSSP (ITT window #1) ====
    std::vector<double> times_bmssp; times_bmssp.reserve(N);
    {
        std::vector<double> db(V, INF);
        auto index_of = [](Key k) { return (std::size_t)k; };

        ITT_RESUME();
        for (int s : sources) {
            std::fill(db.begin(), db.end(), INF);
            db[(std::size_t)s] = 0.0;
            std::vector<Key> S = { s };
            auto t0 = Clock::now();
            auto out = sssp::bmssp<Key>(l, INF, S, adj, db, index_of, M, K_work);
            auto t1 = Clock::now(); (void)out;
            times_bmssp.push_back(std::chrono::duration<double>(t1 - t0).count());
        }
        ITT_PAUSE();
    }
    const Stats st_bmssp = stats_of(times_bmssp);

    // ==== מדידת Dijkstra (ITT window #2) ====
    std::vector<double> times_dijk; times_dijk.reserve(N);
    {
        ITT_RESUME();
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
        ITT_PAUSE();
    }
    const Stats st_dijk = stats_of(times_dijk);

    // ==== טבלת השוואה ====
    logsys::current_level() = logsys::Level::INFO;
    print_header();
    print_row("BMSSP", V, E, (std::size_t)sources.size(), st_bmssp);
    print_row("Dijkstra", V, E, (std::size_t)sources.size(), st_dijk);

    // שורת סיכום ידידותית ללוג הקיים (אם תרצי לשמור תיעוד כמו קודם)
    LOG_INFO() << "[BMSSP] N=" << times_bmssp.size()
        << " mean=" << st_bmssp.mean
        << " median=" << st_bmssp.median
        << " p95=" << st_bmssp.p95 << " s";
    LOG_INFO() << "[Dijkstra] N=" << times_dijk.size()
        << " mean=" << st_dijk.mean
        << " median=" << st_dijk.median
        << " p95=" << st_dijk.p95 << " s";

    return 0;
}
