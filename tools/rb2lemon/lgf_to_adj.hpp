#pragma once
#include <vector>
#include <utility>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>

using Adj = std::vector<std::vector<std::pair<int, double>>>;

inline Adj loadAdjFromDirectedLGF(const std::string& path, bool skipNeg = true)
{
    using Digraph = lemon::ListDigraph;

    Digraph g;
    Digraph::ArcMap<double> w(g);
    Digraph::NodeMap<int>   nid(g, -1);

    lemon::digraphReader(g, path)
        .nodeMap("id", nid)
        .arcMap("weight", w)
        .run(); 

    std::vector<int> ids;
    ids.reserve(lemon::countNodes(g));
    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    std::unordered_map<int, int> id2idx;
    id2idx.reserve(ids.size());
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

inline Adj lgf_to_adj(const std::string& path, bool skipNeg = true) {
    return loadAdjFromDirectedLGF(path, skipNeg);
}
