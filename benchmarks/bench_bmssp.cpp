// bench_bmssp.cpp
#include "bench_common.hpp"
#include "sssp/algorithms/bmssp.hpp"

#include <filesystem>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <random>
#include <vector>
#include <cstdlib>   // getenv

// ----- ITT helpers -----
#if HAS_ITT
#include <ittnotify.h>
#define ITT_RESUME() __itt_resume()
#define ITT_PAUSE()  __itt_pause()
#else
#define ITT_RESUME() ((void)0)
#define ITT_PAUSE()  ((void)0)
#endif

// ---------------- helpers (CLI, קבצי מקורות) ----------------
static std::string cli_get_arg(int argc, char** argv, const std::string& keyEq) {
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s.rfind(keyEq, 0) == 0) return s.substr(keyEq.size());
    }
    return {};
}

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
// ---------------------------------------------------------------------

int main(int argc, char** argv) {
    using clk = std::chrono::steady_clock;
    logsys::current_level() = logsys::Level::ERROR;
    ITT_PAUSE();

    // ===== נתיב RB: חובה --rb=PATH או ENV:SSSP_RB_PATH; אין ברירת מחדל =====
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
            if (exists_file(p)) {
                rb_path = p.string();
            }
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

    const std::size_t N = (argc > 2 ? std::stoul(argv[2]) : 50);

    // ===== המרה RB->LGF (מחוץ למדידה) =====
    const std::size_t dotPos = rb_path.find_last_of('.');
    const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
    const std::string lgf_path = base + "_graph.lgf";
    const std::string graphml_path = base + "_graph.graphml";

    if (!std::filesystem::exists(lgf_path)) {
        CoutSilencer quiet;
        rbconv::RBToLemonConverter conv;
        if (!conv.readRutherfordBoeing(rb_path)) {
            LOG_ERROR() << "Failed to read RB file: " << rb_path;
            return 1;
        }
        conv.saveToLemonFormat(lgf_path);
        // conv.saveToGraphML(graphml_path);
    }
    else {
        LOG_INFO() << "LGF file already exists, skipping conversion.";
    }

    // ===== בניית Adj (מחוץ למדידה) =====
    std::vector<int> idx2id;
    Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
    if (adj.empty()) { LOG_ERROR() << "Adj empty"; return 1; }

    // ===== פרמטרים ל-BMSSP =====
    const std::size_t n = adj.size();
    const double ln_n = std::log((double)n);
    int k = (int)std::floor(std::cbrt(ln_n)); if (k < 1) k = 1;
    int t = (int)std::floor(std::pow(ln_n, 2.0 / 3.0)); if (t < 1) t = 1;
    int l = (int)std::ceil(ln_n / (double)t); if (l < 1) l = 1;
    int exp_int = (l - 1) * t; if (exp_int < 0) exp_int = 0;
    std::size_t M = (std::size_t)std::pow(2.0, (double)exp_int); if (M == 0) M = 1;
    const std::size_t K_work = std::max<std::size_t>(M, 1u << 16);

    // ===== בחירת מקורות =====
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
        LOG_INFO() << "Sources file created: " << sources_path << " (" << sources.size() << " vertices).";
    }
    else {
        LOG_INFO() << "Using sources from file: " << sources_path << " (" << sources.size() << " vertices).";
    }

    // ===== מדידה =====
    std::vector<double> times; times.reserve(N);
    std::vector<double> db(n, INF);
    auto index_of = [](Key k) { return (std::size_t)k; };

    // Warmup (מחוץ למדידה)
    if (!sources.empty()) {
        int s0 = sources[0];
        std::fill(db.begin(), db.end(), INF);
        db[(size_t)s0] = 0.0;
        std::vector<Key> S0 = { s0 };
        auto t0 = clk::now();
        auto out0 = sssp::bmssp<Key>(l, INF, S0, adj, db, index_of, M, K_work);
        auto t1 = clk::now(); (void)out0;
        (void)std::chrono::duration<double>(t1 - t0).count();
    }

    ITT_RESUME();
    for (int s : sources) {
        std::fill(db.begin(), db.end(), INF);
        db[(size_t)s] = 0.0;
        std::vector<Key> S = { s };
        auto t0 = clk::now();
        auto out = sssp::bmssp<Key>(l, INF, S, adj, db, index_of, M, K_work);
        auto t1 = clk::now(); (void)out;
        times.push_back(std::chrono::duration<double>(t1 - t0).count());
    }
    ITT_PAUSE();

    logsys::current_level() = logsys::Level::INFO;
    const auto st = stats_of(times);
    LOG_INFO() << "[BMSSP] N=" << times.size()
        << " mean=" << st.mean
        << " median=" << st.median
        << " p95=" << st.p95 << " s";
    return 0;
}
