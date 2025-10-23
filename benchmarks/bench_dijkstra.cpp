//#include "bench_common.hpp"
//#include <filesystem>
//
//
//int main(int argc, char** argv) {
//    logsys::current_level() = logsys::Level::INFO;
//
//    std::string rb_path = (argc > 1 ? argv[1]
//        : "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\webbase-1M.rb");
//    std::size_t N = (argc > 2 ? std::stoul(argv[2]) : 50);
//
//    // המרה RB->LGF (לא נמדד; מושתק)
//    const std::size_t dotPos = rb_path.find_last_of('.');
//    const std::string base = (dotPos == std::string::npos) ? rb_path : rb_path.substr(0, dotPos);
//    const std::string lgf_path = base + "_graph.lgf";
//    const std::string graphml_path = base + "_graph.graphml";
//    if (!std::filesystem::exists(lgf_path)) {
//        CoutSilencer quiet;
//        rbconv::RBToLemonConverter conv;
//        if (!conv.readRutherfordBoeing(rb_path)) {
//            LOG_ERROR() << "Failed to read RB file.";
//            return 1;
//        }
//        conv.saveToLemonFormat(lgf_path);
//        conv.saveToGraphML(graphml_path);
//    }
//    else {
//        LOG_INFO() << "LGF file already exists, skipping conversion.";
//    }
//    // Adj + idx2id
//    std::vector<int> idx2id;
//    Adj adj = loadAdjFromDirectedLGF(lgf_path, &idx2id, /*skipNeg=*/true);
//    if (adj.empty()) { LOG_ERROR() << "Adj empty"; return 1; }
//
//    // LEMON build (פעם אחת)
//    Digraph g; Digraph::ArcMap<double> w(g); Digraph::NodeMap<int> nid(g, -1);
//    build_lemon_from_lgf(lgf_path, g, w, nid);
//    auto node_by_id = build_node_by_id(g, nid);
//    lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(g, w);
//
//    // בוחרים N מקורות
//    auto top = top_k_by_outdeg(adj, std::min<std::size_t>(N, 200));
//    auto sources = make_source_set(adj, N, top);
//
//    // מדידה
//    std::vector<double> times; times.reserve(N);
//
//    // warmup
//    {
//        int s = sources[0];
//        auto it = node_by_id.find(idx2id[s]);
//        if (it != node_by_id.end()) {
//            auto t0 = clk::now();
//            dij.run(it->second);
//            auto t1 = clk::now();
//            (void)std::chrono::duration<double>(t1 - t0).count();
//        }
//    }
//
//    for (int s : sources) {
//        auto it = node_by_id.find(idx2id[s]);
//        if (it == node_by_id.end()) continue;
//        auto t0 = clk::now();
//        dij.run(it->second);
//        auto t1 = clk::now();
//        times.push_back(std::chrono::duration<double>(t1 - t0).count());
//    }
//
//    auto st = stats_of(times);
//    LOG_INFO() << "[Dijkstra] N=" << N
//        << " mean=" << st.mean
//        << " median=" << st.median
//        << " p95=" << st.p95 << " s";
//    return 0;
//}



#include "bench_common.hpp"
#include <filesystem>

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/dijkstra.h>

using Digraph = lemon::ListDigraph;
using Clock = std::chrono::high_resolution_clock;

int main(int argc, char** argv) {
    // לכבות רעשים בזמן המדידה
    logsys::current_level() = logsys::Level::ERROR;

    // פרמטרים
    std::string rb_path = (argc > 1 ? argv[1]
        : "C:\\Users\\user1\\Desktop\\directed-single-source-shortest-paths\\data\\webbase-1M.rb");
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
    for (int s : sources) {
        auto it = node_by_id.find(idx2id[s]);
        if (it == node_by_id.end()) continue;
        times.push_back(run_once(it->second));
    }

    auto st = stats_of(times);

    // שורת סיכום
    logsys::current_level() = logsys::Level::INFO;
    LOG_INFO() << "[Dijkstra] N=" << N
        << " mean=" << st.mean
        << " median=" << st.median
        << " p95=" << st.p95 << " s";
    return 0;
}
