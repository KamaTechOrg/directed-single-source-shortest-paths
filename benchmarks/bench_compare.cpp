// benchmarks/bench_compare.cpp
#include "bench_common.hpp"         // כולל את כל העזרים ששלחת: Adj/Key/INF/Digraph/... + stats_of
#include "sssp/algorithms/bmssp.hpp"

#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>

// ---------- הדפסה יפה (מותאם ל-Stats: mean/median/p95) ----------
static void print_stats_row(const std::string& name, const Stats& s, std::size_t N) {
    std::cout << std::left << std::setw(10) << name
        << " N=" << std::setw(6) << N
        << " mean=" << std::setw(12) << s.mean
        << " median=" << std::setw(12) << s.median
        << " p95=" << std::setw(12) << s.p95
        << '\n';
}

// ---------- main ----------
int main(int argc, char** argv) {
    logsys::current_level() = logsys::Level::INFO; // הדפס רק סיכום

    // קלט
    const std::string rb_path = (argc > 1 ? argv[1]
        : "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\webbase-1M.rb");
    const std::size_t N = (argc > 2 ? std::stoul(argv[2]) : 500);
    const bool skipNeg = true; // משקלים חיוביים בלבד

    // המרה RB->LGF (מושתק ולא במדידה)
    const std::size_t dotPos = rb_path.find_last_of('.');
    const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
    const std::string lgf_path = base + "_graph.lgf";
    const std::string graphml_path = base + "_graph.graphml";
    {
        CoutSilencer quiet;
        rbconv::RBToLemonConverter conv;
        if (!conv.readRutherfordBoeing(rb_path)) {
            LOG_ERROR() << "Failed to read RB file.";
            return 1;
        }
        conv.saveToLemonFormat(lgf_path);
        conv.saveToGraphML(graphml_path);
    }

    // טעינת LGF ל־Adj (BMSSP) + אינדקסים
    std::vector<int> idx2id;
    Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/skipNeg);
    if (adj.empty()) { LOG_ERROR() << "Adj empty"; return 1; }
    const std::size_t n = adj.size();

    // בניית גרף LEMON פעם אחת (לדייקסטרה)
    Digraph g; Digraph::ArcMap<double> w(g); Digraph::NodeMap<int> nid(g, -1);
    build_lemon_from_lgf(lgf_path, g, w, nid);
    auto node_by_id = build_node_by_id(g, nid);
    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);

    // בחירת מקורות: top outdegree + רנדומלי
    auto top = top_k_by_outdeg(adj, std::min<std::size_t>(N, 200));
    auto sources = make_source_set(adj, N, top);

    // מיפוי מקורות ל־LEMON נודס (להפחתת overhead)
    std::vector<Digraph::Node> src_nodes;
    src_nodes.reserve(sources.size());
    for (int s : sources) {
        auto it = node_by_id.find(idx2id[s]);
        if (it != node_by_id.end()) src_nodes.push_back(it->second);
    }
    if (src_nodes.empty()) { LOG_ERROR() << "No valid sources"; return 1; }

    // -------- BMSSP: פרמטרים עפ"י ln(n) --------
    const double ln_n = std::log((double)n);
    int k = (int)std::floor(std::cbrt(ln_n)); if (k < 1) k = 1;
    int t = (int)std::floor(std::pow(ln_n, 2.0 / 3.0)); if (t < 1) t = 1;
    int l = (int)std::ceil(ln_n / (double)t); if (l < 1) l = 1;
    int exp_int = (l - 1) * t; if (exp_int < 0) exp_int = 0;
    std::size_t M = (std::size_t)std::pow(2.0, (double)exp_int); if (M == 0) M = 1;
    const std::size_t K_work = std::max<std::size_t>(M, 1u << 16);

    // -------- Warmup דומה לשני האלגוריתמים --------
    {
        auto t0 = clk::now(); dij.run(src_nodes[0]); auto t1 = clk::now();
        (void)std::chrono::duration<double>(t1 - t0).count();
    }
    {
        std::vector<double> db(n, INF);
        int s = sources[0];
        db[(size_t)s] = 0.0;
        std::vector<Key> S = { s };
        auto index_of = [](Key k) { return (std::size_t)k; };
        auto t0 = clk::now();
        auto out = sssp::bmssp<Key>(l, INF, S, adj, db, index_of, M, K_work);
        auto t1 = clk::now(); (void)out;
        (void)std::chrono::duration<double>(t1 - t0).count();
    }

    // --------- מדידה: Dijkstra ---------
    std::vector<double> times_d;
    times_d.reserve(src_nodes.size());
    for (auto node : src_nodes) {
        auto t0 = clk::now();
        dij.run(node);
        auto t1 = clk::now();
        times_d.push_back(std::chrono::duration<double>(t1 - t0).count());
    }
    const Stats st_d = stats_of(times_d);

    // --------- מדידה: BMSSP ---------
    std::vector<double> times_b; times_b.reserve(sources.size());
    std::vector<double> db(n, INF);
    auto index_of = [](Key k) { return (std::size_t)k; };
    volatile double sink = 0.0; // צריכת תוצר למניעת DCE
    for (int s : sources) {
        std::fill(db.begin(), db.end(), INF);
        db[(size_t)s] = 0.0;
        std::vector<Key> S = { s };
        auto t0 = clk::now();
        auto out = sssp::bmssp<Key>(l, INF, S, adj, db, index_of, M, K_work);
        auto t1 = clk::now(); (void)out;
        double dt = std::chrono::duration<double>(t1 - t0).count();
        times_b.push_back(dt);
        sink += db[(size_t)s];
    }
    (void)sink;
    const Stats st_b = stats_of(times_b);

    // --------- סיכום ---------
    std::cout << "Graph stats: |V|=" << n
        << "  (LEMON nodes=" << lemon::countNodes(g)
        << ", arcs=" << lemon::countArcs(g) << ")\n";
    std::cout << "Results (seconds):\n";
    print_stats_row("Dijkstra", st_d, times_d.size());
    print_stats_row("BMSSP", st_b, times_b.size());

    // לוג קצר (מותאם ל-Stats שלך)
    LOG_INFO() << "[Dijkstra] N=" << times_d.size()
        << " mean=" << st_d.mean
        << " median=" << st_d.median
        << " p95=" << st_d.p95;

    LOG_INFO() << "[BMSSP] N=" << times_b.size()
        << " mean=" << st_b.mean
        << " median=" << st_b.median
        << " p95=" << st_b.p95;

    return 0;
}
