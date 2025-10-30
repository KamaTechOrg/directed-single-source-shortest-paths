//מדידת זמן בפרופילינג אמיתית יותר ללא זמן המרת הגרף
#include "bench_common.hpp"
#include <filesystem>

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/dijkstra.h>

#include "config_paths.hpp"


#ifdef HAS_ITT
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

    // פרמטרים
    std::string rb_path = cli_get_arg(argc, argv, "--rb=");
    if (rb_path.empty()) {
        rb_path = std::string(SSSP_DEFAULT_RB);
    }

    std::size_t N = (argc > 2 ? std::stoul(argv[2]) : 50);

    // מסלולי קבצים
    const std::size_t dotPos = rb_path.find_last_of('.');
    const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
    const std::string lgf_path = base + "_graph.lgf";
    const std::string graphml_path = base + "_graph.graphml";

    // המרה RB->LGF אם צריך (מחוץ למדידה)
    if (!std::filesystem::exists(lgf_path)) {
        CoutSilencer quiet;
        rbconv::RBToLemonConverter conv;
        if (!conv.readRutherfordBoeing(rb_path)) {
            LOG_ERROR() << "Failed to read RB file.";
            return 1;
        }
        conv.saveToLemonFormat(lgf_path);
        conv.saveToGraphML(graphml_path);
    }
    else {
        LOG_INFO() << "LGF file already exists, skipping conversion.";
    }

    // Adj לבחירת מקורות (מחוץ למדידה)
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


    auto st = stats_of(times);

    // שורת סיכום
    logsys::current_level() = logsys::Level::INFO;
    LOG_INFO() << "[Dijkstra] N=" << N
        << " mean=" << st.mean
        << " median=" << st.median
        << " p95=" << st.p95 << " s";
    return 0;
}
