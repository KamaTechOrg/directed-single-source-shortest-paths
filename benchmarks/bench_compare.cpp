    //// benchmarks/bench_compare.cpp
    //#include "bench_common.hpp"         // כולל את כל העזרים ששלחת: Adj/Key/INF/Digraph/... + stats_of
    //#include "sssp/algorithms/bmssp.hpp"
    //
    //#include <algorithm>
    //#include <numeric>
    //#include <cmath>
    //#include <iomanip>
    //
    //// ---------- הדפסה יפה (מותאם ל-Stats: mean/median/p95) ----------
    //static void print_stats_row(const std::string& name, const Stats& s, std::size_t N) {
    //    std::cout << std::left << std::setw(10) << name
    //        << " N=" << std::setw(6) << N
    //        << " mean=" << std::setw(12) << s.mean
    //        << " median=" << std::setw(12) << s.median
    //        << " p95=" << std::setw(12) << s.p95
    //        << '\n';
    //}
    //
    //// ---------- main ----------
    //int main(int argc, char** argv) {
    //    logsys::current_level() = logsys::Level::INFO; // הדפס רק סיכום
    //
    //    // קלט
    //    const std::string rb_path = (argc > 1 ? argv[1]
    //        : "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\patents_main.rb");
    //    const std::size_t N = (argc > 2 ? std::stoul(argv[2]) : 500);
    //    const bool skipNeg = true; // משקלים חיוביים בלבד
    //
    //    // המרה RB->LGF (מושתק ולא במדידה)
    //    const std::size_t dotPos = rb_path.find_last_of('.');
    //    const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
    //    const std::string lgf_path = base + "_graph.lgf";
    //    const std::string graphml_path = base + "_graph.graphml";
    //    {
    //        CoutSilencer quiet;
    //        rbconv::RBToLemonConverter conv;
    //        if (!conv.readRutherfordBoeing(rb_path)) {
    //            LOG_ERROR() << "Failed to read RB file.";
    //            return 1;
    //        }
    //        conv.saveToLemonFormat(lgf_path);
    //        conv.saveToGraphML(graphml_path);
    //    }
    //
    //    // טעינת LGF ל־Adj (BMSSP) + אינדקסים
    //    std::vector<int> idx2id;
    //    Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/skipNeg);
    //    if (adj.empty()) { LOG_ERROR() << "Adj empty"; return 1; }
    //    const std::size_t n = adj.size();
    //
    //    // בניית גרף LEMON פעם אחת (לדייקסטרה)
    //    Digraph g; Digraph::ArcMap<double> w(g); Digraph::NodeMap<int> nid(g, -1);
    //    build_lemon_from_lgf(lgf_path, g, w, nid);
    //    auto node_by_id = build_node_by_id(g, nid);
    //    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
    //
    //    // בחירת מקורות: top outdegree + רנדומלי
    //    auto top = top_k_by_outdeg(adj, std::min<std::size_t>(N, 200));
    //    auto sources = make_source_set(adj, N, top);
    //
    //    // מיפוי מקורות ל־LEMON נודס (להפחתת overhead)
    //    std::vector<Digraph::Node> src_nodes;
    //    src_nodes.reserve(sources.size());
    //    for (int s : sources) {
    //        auto it = node_by_id.find(idx2id[s]);
    //        if (it != node_by_id.end()) src_nodes.push_back(it->second);
    //    }
    //    if (src_nodes.empty()) { LOG_ERROR() << "No valid sources"; return 1; }
    //
    //    // -------- BMSSP: פרמטרים עפ"י ln(n) --------
    //    const double ln_n = std::log((double)n);
    //    int k = (int)std::floor(std::cbrt(ln_n)); if (k < 1) k = 1;
    //    int t = (int)std::floor(std::pow(ln_n, 2.0 / 3.0)); if (t < 1) t = 1;
    //    int l = (int)std::ceil(ln_n / (double)t); if (l < 1) l = 1;
    //    int exp_int = (l - 1) * t; if (exp_int < 0) exp_int = 0;
    //    std::size_t M = (std::size_t)std::pow(2.0, (double)exp_int); if (M == 0) M = 1;
    //    const std::size_t K_work = std::max<std::size_t>(M, 1u << 16);
    //
    //    // -------- Warmup דומה לשני האלגוריתמים --------
    //    {
    //        auto t0 = clk::now(); dij.run(src_nodes[0]); auto t1 = clk::now();
    //        (void)std::chrono::duration<double>(t1 - t0).count();
    //    }
    //    {
    //        std::vector<double> db(n, INF);
    //        int s = sources[0];
    //        db[(size_t)s] = 0.0;
    //        std::vector<Key> S = { s };
    //        auto index_of = [](Key k) { return (std::size_t)k; };
    //        auto t0 = clk::now();
    //        auto out = sssp::bmssp<Key>(l, INF, S, adj, db, index_of, M, K_work);
    //        auto t1 = clk::now(); (void)out;
    //        (void)std::chrono::duration<double>(t1 - t0).count();
    //    }
    //
    //    // --------- מדידה: Dijkstra ---------
    //    std::vector<double> times_d;
    //    times_d.reserve(src_nodes.size());
    //    for (auto node : src_nodes) {
    //        auto t0 = clk::now();
    //        dij.run(node);
    //        auto t1 = clk::now();
    //        times_d.push_back(std::chrono::duration<double>(t1 - t0).count());
    //    }
    //    const Stats st_d = stats_of(times_d);
    //
    //    // --------- מדידה: BMSSP ---------
    //    std::vector<double> times_b; times_b.reserve(sources.size());
    //    std::vector<double> db(n, INF);
    //    auto index_of = [](Key k) { return (std::size_t)k; };
    //    volatile double sink = 0.0; // צריכת תוצר למניעת DCE
    //    for (int s : sources) {
    //        std::fill(db.begin(), db.end(), INF);
    //        db[(size_t)s] = 0.0;
    //        std::vector<Key> S = { s };
    //        auto t0 = clk::now();
    //        auto out = sssp::bmssp<Key>(l, INF, S, adj, db, index_of, M, K_work);
    //        auto t1 = clk::now(); (void)out;
    //        double dt = std::chrono::duration<double>(t1 - t0).count();
    //        times_b.push_back(dt);
    //        sink += db[(size_t)s];
    //    }
    //    (void)sink;
    //    const Stats st_b = stats_of(times_b);
    //
    //    // --------- סיכום ---------
    //    std::cout << "Graph stats: |V|=" << n
    //        << "  (LEMON nodes=" << lemon::countNodes(g)
    //        << ", arcs=" << lemon::countArcs(g) << ")\n";
    //    std::cout << "Results (seconds):\n";
    //    print_stats_row("Dijkstra", st_d, times_d.size());
    //    print_stats_row("BMSSP", st_b, times_b.size());
    //
    //    // לוג קצר (מותאם ל-Stats שלך)
    //    LOG_INFO() << "[Dijkstra] N=" << times_d.size()
    //        << " mean=" << st_d.mean
    //        << " median=" << st_d.median
    //        << " p95=" << st_d.p95;
    //
    //    LOG_INFO() << "[BMSSP] N=" << times_b.size()
    //        << " mean=" << st_b.mean
    //        << " median=" << st_b.median
    //        << " p95=" << st_b.p95;
    //
    //    return 0;
    //}



    //#pragma once
    //#include <algorithm>
    //#include <chrono>
    //#include <cmath>
    //#include <fstream>
    //#include <limits>
    //#include <random>
    //#include <sstream>
    //#include <string>
    //#include <unordered_map>
    //#include <unordered_set>
    //#include <utility>
    //#include <vector>

    //#include "logger.hpp"      // include/sssp/logger.hpp
    //#include "rb_to_lemon.hpp" // tools/rb2lemon/rb_to_lemon.hpp

    //#include <lemon/list_graph.h>
    //#include <lemon/lgf_reader.h>
    //#include <lemon/dijkstra.h>

    //#ifndef HAS_ITT
    //#define HAS_ITT 0
    //#endif

    //#if HAS_ITT
    //#include <ittnotify.h>
    //#define ITT_RESUME() __itt_resume()
    //#define ITT_PAUSE()  __itt_pause()
    //#else
    //#define ITT_RESUME() ((void)0)
    //#define ITT_PAUSE()  ((void)0)
    //#endif


    //using clk = std::chrono::steady_clock;
    //using Key = int;
    //using Adj = std::vector<std::vector<std::pair<Key, double>>>;
    //static const double INF = std::numeric_limits<double>::infinity();
    //using Digraph = lemon::ListDigraph;

    //// ---- משתיק הדפסות צד שלישי ----
    //struct CoutSilencer {
    //    std::streambuf* old_cout = nullptr;
    //    std::streambuf* old_cerr = nullptr;
    //    std::ofstream   nullfile;
    //    CoutSilencer() {
    //#ifdef _WIN32
    //        nullfile.open("NUL");
    //#else
    //        nullfile.open("/dev/null");
    //#endif
    //        old_cout = std::cout.rdbuf(nullfile.rdbuf());
    //        old_cerr = std::cerr.rdbuf(nullfile.rdbuf());
    //    }
    //    ~CoutSilencer() {
    //        std::cout.rdbuf(old_cout);
    //        std::cerr.rdbuf(old_cerr);
    //    }
    //};

    //// ---- טעינת LGF => Adj + idx2id ----
    //inline Adj loadAdjFromDirectedLGF(const std::string& path,
    //    std::vector<int>* out_idx2id = nullptr,
    //    bool skipNeg = true)
    //{
    //    Digraph g;
    //    Digraph::ArcMap<double> w(g);
    //    Digraph::NodeMap<int>   nid(g, -1);
    //    lemon::digraphReader(g, path).nodeMap("id", nid).arcMap("weight", w).run();

    //    std::vector<int> ids; ids.reserve(lemon::countNodes(g));
    //    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
    //    std::sort(ids.begin(), ids.end());
    //    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    //    std::unordered_map<int, int> id2idx; id2idx.reserve(ids.size());
    //    for (int i = 0; i < (int)ids.size(); ++i) id2idx[ids[i]] = i;
    //    if (out_idx2id) *out_idx2id = ids;

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

    //// ---- K בעלי דרגת יציאה גבוהה ----
    //inline std::vector<int> top_k_by_outdeg(const Adj& adj, std::size_t K = 100) {
    //    if (adj.empty()) return {};
    //    if (K == 0) K = 1;
    //    if (K > adj.size()) K = adj.size();

    //    std::vector<std::pair<std::size_t, int>> best; best.reserve(K);
    //    auto push = [&](std::size_t deg, int idx) {
    //        if (best.size() < K) best.emplace_back(deg, idx);
    //        else {
    //            std::size_t m = 0;
    //            for (std::size_t i = 1; i < best.size(); ++i)
    //                if (best[i].first < best[m].first) m = i;
    //            if (deg > best[m].first) best[m] = { deg, idx };
    //        }
    //        };
    //    for (std::size_t i = 0; i < adj.size(); ++i) push(adj[i].size(), (int)i);

    //    std::sort(best.begin(), best.end(),
    //        [](const auto& a, const auto& b) {
    //            if (a.first != b.first) return a.first > b.first;
    //            return a.second < b.second;
    //        });

    //    std::vector<int> res; res.reserve(best.size());
    //    for (auto& p : best) res.push_back(p.second);
    //    return res;
    //}

    //// ---- בחירת סט מקורות: חלק top-K, השאר רנדומלי ----
    //inline std::vector<int> make_source_set(const Adj& adj,
    //    std::size_t N,
    //    const std::vector<int>& preferTop = {})
    //{
    //    std::vector<int> srcs; srcs.reserve(N);
    //    for (int s : preferTop) { if (srcs.size() == N) break; srcs.push_back(s); }

    //    if (adj.empty()) return srcs;

    //    std::mt19937 rng(12345);
    //    std::uniform_int_distribution<int> dist(0, (int)adj.size() - 1);
    //    std::unordered_set<int> used(srcs.begin(), srcs.end());

    //    while (srcs.size() < N) {
    //        int s = dist(rng);
    //        if (used.insert(s).second) srcs.push_back(s);
    //    }
    //    return srcs;
    //}

    //// ---- בניית גרף LEMON מה-LGF ----
    //inline void build_lemon_from_lgf(const std::string& lgf_path,
    //    Digraph& g,
    //    Digraph::ArcMap<double>& w,
    //    Digraph::NodeMap<int>& nid)
    //{
    //    lemon::digraphReader(g, lgf_path).nodeMap("id", nid).arcMap("weight", w).run();
    //}

    //inline std::unordered_map<int, Digraph::Node>
    //build_node_by_id(const Digraph& g, const Digraph::NodeMap<int>& nid)
    //{
    //    std::unordered_map<int, Digraph::Node> mp;
    //    mp.reserve(lemon::countNodes(g));
    //    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) mp.emplace(nid[n], n);
    //    return mp;
    //}

    //// ---- סטטיסטיקות ----
    //struct Stats { double mean, median, p95; };
    //inline Stats stats_of(std::vector<double> v) {
    //    if (v.empty()) return { 0,0,0 };
    //    double sum = 0; for (double x : v) sum += x;
    //    const double mean = sum / v.size();

    //    auto mid = v.begin() + v.size() / 2;
    //    std::nth_element(v.begin(), mid, v.end());
    //    const double med = *mid;

    //    std::size_t p95i = (std::size_t)std::floor(0.95 * (v.size() - 1));
    //    std::nth_element(v.begin(), v.begin() + p95i, v.end());
    //    const double p95 = v[p95i];

    //    return { mean, med, p95 };
    //}





// benchmarks/bench_compare.cpp
#include "bench_common.hpp"
#include "sssp/algorithms/bmssp.hpp"

#include <filesystem>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <random>
#include <unordered_map>
#include <iomanip>      // לטבלה יפה

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/dijkstra.h>

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
// בונה רשימת מקורות דטרמיניסטית: top-by-outdeg ואז השלמה לפי אינדקס
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
    // שקט בזמן בנצ'מארק
    logsys::current_level() = logsys::Level::ERROR;

    // ==== קלט: RB + N ====
    const std::string rb_path = (argc > 1 ? argv[1]
        //: "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\patents_main.rb");
        : "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\webbase-1M.rb");
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
