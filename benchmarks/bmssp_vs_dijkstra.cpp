    // bmssp_vs_dijkstra_benchmark.cpp
    #include <benchmark/benchmark.h>
    #include <vector>
    #include <utility>
    #include <string>
    #include <unordered_map>
    #include <algorithm>
    #include <limits>
    #include <memory>

#include "rb_to_lemon.hpp"
#include "sssp/algorithms/bmssp.hpp"

    #include <lemon/list_graph.h>
    #include <lemon/lgf_reader.h>
    #include <lemon/dijkstra.h>

    using Key = int;
    using Adj = std::vector<std::vector<std::pair<Key, double>>>;
    constexpr double INF = std::numeric_limits<double>::infinity();

    // ---------- helpers: load LGF to adjacency ----------
    static Adj loadAdjFromDirectedLGF(const std::string& path, bool skipNeg = true) {
        using Digraph = lemon::ListDigraph;
        Digraph g; Digraph::ArcMap<double> w(g); Digraph::NodeMap<int> nid(g, -1);

        lemon::digraphReader(g, path)
            .nodeMap("id", nid)
            .arcMap("weight", w)
            .run();

        std::vector<int> ids; ids.reserve(lemon::countNodes(g));
        for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

        std::unordered_map<int, int> id2idx; id2idx.reserve(ids.size());
        for (int i = 0; i < (int)ids.size(); ++i) id2idx[ids[i]] = i;

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

    // ---------- dataset singleton: RB->LGF נטען פעם אחת ----------
    struct Dataset {
        std::string rb_path;
        std::string lgf_path;

        Adj adj;

        using Digraph = lemon::ListDigraph;
        Digraph g;
        std::unique_ptr<Digraph::ArcMap<double>> w;
        std::unique_ptr<Digraph::NodeMap<int>> nid;
        Digraph::Node src_node = lemon::INVALID;
        int source_id = 0;
        std::unordered_map<int, int> id2idx;

        static Dataset& get() { static Dataset ds; return ds; }

    private:
        Dataset() {
    #ifndef RB_PATH
    #define RB_PATH "C:/Users/user1/Desktop/directed-single-source-shortest-paths/data/EAT_RS.rb"
    #endif
            rb_path = RB_PATH;

            const std::size_t dotPos = rb_path.find_last_of('.');
            lgf_path = (dotPos == std::string::npos)
                ? rb_path
                : rb_path.substr(0, dotPos) + "_graph.lgf";

            {   // יצירת LGF אם צריך (Idempotent)
                rbconv::RBToLemonConverter conv;
                if (conv.readRutherfordBoeing(rb_path)) {
                    conv.saveToLemonFormat(lgf_path);
                }
            }

            adj = loadAdjFromDirectedLGF(lgf_path, /*skipNeg=*/true);

            w = std::make_unique<Digraph::ArcMap<double>>(g);
            nid = std::make_unique<Digraph::NodeMap<int>>(g, -1);
            lemon::digraphReader(g, lgf_path)
                .nodeMap("id", *nid)
                .arcMap("weight", *w)
                .run();

            source_id = 0;
            for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) {
                if ((*nid)[n] == source_id) { src_node = n; break; }
            }

            std::vector<int> ids; ids.reserve(lemon::countNodes(g));
            for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back((*nid)[n]);
            std::sort(ids.begin(), ids.end());
            ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
            id2idx.reserve(ids.size());
            for (int i = 0; i < (int)ids.size(); ++i) id2idx[ids[i]] = i;
        }
    };

    // ---------- BMSSP benchmark ----------
    static void BM_BMSSP(benchmark::State& state) {
        auto& ds = Dataset::get();
        const std::size_t n = ds.adj.size();
        const Key src = 0;
        auto index_of = [](Key k) -> std::size_t { return static_cast<std::size_t>(k); };

        std::size_t M = 1ull << 16;
        std::size_t K = 1ull << 16;

        const int Barg = static_cast<int>(state.range(0));
        const double B = (Barg < 0) ? INF : static_cast<double>(Barg);

        for (auto _ : state) {
            std::vector<double> db(n, INF);
            db[static_cast<std::size_t>(src)] = 0.0;
            std::vector<Key> S = { src };

            auto out = sssp::bmssp<Key>(/*l=*/2, /*B=*/B, S, ds.adj, db, index_of, M, K);
            benchmark::DoNotOptimize(out.Bprime);
            benchmark::DoNotOptimize(db.data());
        }
        state.SetLabel((Barg < 0) ? "B=INF" : ("B=" + std::to_string(Barg)));
    }
    BENCHMARK(BM_BMSSP)
    ->ArgName("B")
    ->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Arg(-1); // -1 => INF

    // ---------- Dijkstra (LEMON) benchmark ----------
    static void BM_Dijkstra_LEMON(benchmark::State& state) {
        auto& ds = Dataset::get();
        using Digraph = Dataset::Digraph;

        for (auto _ : state) {
            lemon::Dijkstra<Digraph, Digraph::ArcMap<double>> dij(ds.g, *ds.w);
            dij.run(ds.src_node);

            std::vector<double> dist(ds.id2idx.size(), INF);
            for (Digraph::NodeIt n(ds.g); n != lemon::INVALID; ++n) {
                int id = (*ds.nid)[n];
                auto it = ds.id2idx.find(id);
                if (it == ds.id2idx.end()) continue;
                std::size_t idx = static_cast<std::size_t>(it->second);
                dist[idx] = dij.reached(n) ? dij.dist(n) : INF;
            }
            benchmark::DoNotOptimize(dist.data());
        }
    }
    BENCHMARK(BM_Dijkstra_LEMON);

