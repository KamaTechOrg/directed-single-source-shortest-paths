// מדידת זמן בפרופילינג אמיתית יותר ללא זמן המרת הגרף
#include "bench_common.hpp"
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <cstdlib>   // getenv

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/dijkstra.h>

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

// ---- CLI helper (לקחת פרמטרים מהשורה) ----
static std::string cli_get_arg(int argc, char** argv, const std::string& keyEq) {
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s.rfind(keyEq, 0) == 0)
            return s.substr(keyEq.size());
    }
    return {};
}

int main(int argc, char** argv) {
    // לכבות רעשים בזמן המדידה
    logsys::current_level() = logsys::Level::ERROR;

    // ===== קבלת נתיב RB: חובה --rb=PATH או ENV:SSSP_RB_PATH; אין חיפוש לאחור =====
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
        try { LOG_INFO() << "CWD=" << std::filesystem::current_path().string(); }
        catch (...) {}
        LOG_ERROR() << "RB path not provided. Use --rb=<path> or set SSSP_RB_PATH.";
        return 1;
    }

    // פרמטר N (כמות מקורות)
    std::size_t N = (argc > 2 ? std::stoul(argv[2]) : 50);

    // ===== מסלולי קבצים =====
    const std::size_t dotPos = rb_path.find_last_of('.');
    const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
    const std::string lgf_path = base + "_graph.lgf";
    const std::string graphml_path = base + "_graph.graphml";

    // ===== המרה RB->LGF אם צריך (מחוץ למדידה) =====
    if (!std::filesystem::exists(lgf_path)) {
        CoutSilencer quiet;
        rbconv::RBToLemonConverter conv;
        if (!conv.readRutherfordBoeing(rb_path)) {
            LOG_ERROR() << "Failed to read RB file: " << rb_path;
            return 1;
        }
        conv.saveToLemonFormat(lgf_path);
        // conv.saveToGraphML(graphml_path); // לא חובה
    }
    else {
        LOG_INFO() << "LGF file already exists, skipping conversion.";
    }

    // ===== Adj לבחירת מקורות (מחוץ למדידה) =====
    std::vector<int> idx2id;
    Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
    if (adj.empty()) { LOG_ERROR() << "Adj empty"; return 1; }

    // --- בניית גרף LEMON פעם אחת (מחוץ למדידה) ---
    Digraph g;
    Digraph::ArcMap<double> w(g);
    Digraph::NodeMap<int>   nid(g, -1);

    lemon::digraphReader(g, lgf_path)
        .nodeMap("id", nid)
        .arcMap("weight", w)
        .run();

    // מיפוי id -> Node (מחוץ למדידה)
    std::unordered_map<int, Digraph::Node> node_by_id;
    node_by_id.reserve(lemon::countNodes(g));
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
        node_by_id.emplace(nid[n], n);
    }

    // מקורות זהים ללוגיקה של BMSSP
    auto top = top_k_by_outdeg(adj, std::min<std::size_t>(N, 200));
    auto sources = make_source_set(adj, N, top);
    if (sources.empty()) { LOG_ERROR() << "No sources selected."; return 1; }

    // אובייקט דייקסטרה ממוחזר
    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);

    auto run_once = [&](Digraph::Node s) -> double {
        dij.init();         // "reset" אמיתי ב-LEMON
        dij.addSource(s);
        auto t0 = Clock::now();
        dij.start();        // מריץ עד סיום
        auto t1 = Clock::now();
        return std::chrono::duration<double>(t1 - t0).count();
        };

    // warmup (לא נאסף)
    {
        int s = sources[0];
        auto it = node_by_id.find(idx2id[s]);
        if (it != node_by_id.end()) (void)run_once(it->second);
    }

    // --- מדידה נקייה של הליבה בלבד ---
    std::vector<double> times; times.reserve(N);
    ITT_RESUME();
    for (int s : sources) {
        auto it = node_by_id.find(idx2id[s]);
        if (it == node_by_id.end()) continue;
        times.push_back(run_once(it->second));
    }
    ITT_PAUSE();

    if (times.empty()) { LOG_ERROR() << "No runs executed."; return 1; }

    auto st = stats_of(times);

    // שורת סיכום
    logsys::current_level() = logsys::Level::INFO;
    LOG_INFO() << "[Dijkstra] N=" << times.size()
        << " mean=" << st.mean
        << " median=" << st.median
        << " p95=" << st.p95 << " s";
    return 0;
}
