#include "bench_common.hpp"
#include "sssp/algorithms/bmssp.hpp"  // הכוונה לאלגוריתם שלך
#include <filesystem>


int main(int argc, char** argv) {
    logsys::current_level() = logsys::Level::INFO; // שיהיה שקט במדידה

    // קלט: קובץ RB ו-N מקורות
    std::string rb_path = (argc > 1 ? argv[1]
        : "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\patents_main.rb");
    std::size_t N = (argc > 2 ? std::stoul(argv[2]) : 50);

    // המרה RB->LGF (לא נמדד; מושתק)
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
    conv.saveToGraphML(graphml_path);
    } else {
         LOG_INFO() << "LGF file already exists, skipping conversion.";
    }

    // בניית Adj פעם אחת
    std::vector<int> idx2id;
    Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
    if (adj.empty()) { LOG_ERROR() << "Adj empty"; return 1; }

    // פרמטרים ל-BMSSP (כמו אצלך)
    const std::size_t n = adj.size();
    const double ln_n = std::log((double)n);
    int k = (int)std::floor(std::cbrt(ln_n)); if (k < 1) k = 1;
    int t = (int)std::floor(std::pow(ln_n, 2.0 / 3.0)); if (t < 1) t = 1;
    int l = (int)std::ceil(ln_n / (double)t); if (l < 1) l = 1;
    int exp_int = (l - 1) * t; if (exp_int < 0) exp_int = 0;
    std::size_t M = (std::size_t)std::pow(2.0, (double)exp_int); if (M == 0) M = 1;
    const std::size_t K_work = std::max<std::size_t>(M, 1u << 16);

    // בוחרים N מקורות (top + רנדומלי)
    auto top = top_k_by_outdeg(adj, std::min<std::size_t>(N, 200));
    auto sources = make_source_set(adj, N, top);

    // מדידה
    std::vector<double> times; times.reserve(N);
    std::vector<double> db(n, INF);
    auto index_of = [](Key k) { return (std::size_t)k; };

    // warmup
    {
        int s = sources[0];
        std::fill(db.begin(), db.end(), INF);
        db[(size_t)s] = 0.0;
        std::vector<Key> S = { s };
        auto t0 = clk::now();
        auto out = sssp::bmssp<Key>(l, INF, S, adj, db, index_of, M, K_work);
        auto t1 = clk::now(); (void)out;
        (void)std::chrono::duration<double>(t1 - t0).count();
    }

    for (int s : sources) {
        std::fill(db.begin(), db.end(), INF);
        db[(size_t)s] = 0.0;
        std::vector<Key> S = { s };
        auto t0 = clk::now();
        auto out = sssp::bmssp<Key>(l, INF, S, adj, db, index_of, M, K_work);
        auto t1 = clk::now(); (void)out;
        times.push_back(std::chrono::duration<double>(t1 - t0).count());
    }

    auto st = stats_of(times);
    LOG_INFO() << "[BMSSP] N=" << N
        << " mean=" << st.mean
        << " median=" << st.median
        << " p95=" << st.p95 << " s";
    return 0;
}
