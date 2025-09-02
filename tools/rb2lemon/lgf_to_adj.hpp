//#pragma once
//#include <vector>
//#include <utility>
//#include <string>
//#include <unordered_map>
//#include <algorithm>
//#include <stdexcept>
//
//// LEMON
//#include <lemon/list_graph.h>
//#include <lemon/lgf_reader.h>
//
//
//
//// קורא LGF *מכוון* ומחזיר adjacency list עם אינדקסים קומפקטיים 0..N-1.
//// דורש שב-@nodes תהיה עמודה "id" ושב-@arcs תהיה עמודה "weight".
//inline std::vector<std::vector<std::pair<int, double>>>
//loadAdjFromDirectedLGF(const std::string& path, bool skipNeg = true)
//{
//    using Digraph = lemon::ListDigraph;
//
//    Digraph g;
//    Digraph::ArcMap<double> w(g);
//    Digraph::NodeMap<int>   nid(g, -1);
//
//    lemon::digraphReader(g, path)
//        .nodeMap("id", nid)
//        .arcMap("weight", w)
//        .run();
//
//    // מיפוי מזהי-קובץ -> אינדקס קומפקטי 0..N-1
//    std::vector<int> ids;
//    ids.reserve(lemon::countNodes(g));
//    for (Digraph::NodeIt n(g); n != lemon::INVALID; ++n) ids.push_back(nid[n]);
//
//    std::sort(ids.begin(), ids.end());
//    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
//
//    std::unordered_map<int, int> id2idx;
//    id2idx.reserve(ids.size());
//    for (int i = 0; i < (int)ids.size(); ++i) id2idx[ids[i]] = i;
//
//    std::vector<std::vector<std::pair<int, double>>> adj(ids.size());
//
//    for (Digraph::ArcIt a(g); a != lemon::INVALID; ++a) {
//        int u = id2idx[nid[g.source(a)]];
//        int v = id2idx[nid[g.target(a)]];
//        double wt = w[a];
//
//        if (u == v) continue;        // בלי לולאות עצמיות
//        if (skipNeg && wt < 0) continue;
//
//        adj[u].push_back({ v, wt });   // מכוון: u -> v
//    }
//    return adj;
//}



#pragma once
#include <vector>
#include <utility>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

// LEMON
#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>

// טיפוס אדג'נסי סטנדרטי לפרויקט
using Adj = std::vector<std::vector<std::pair<int, double>>>;

// קורא LGF *מכוון* ומחזיר adjacency list עם אינדקסים קומפקטיים 0..N-1.
// דורש שב-@nodes תהיה עמודה "id" ושב-@arcs תהיה עמודה "weight".
// זורק lemon::IoError אם הקובץ חסר/שגוי.
inline Adj loadAdjFromDirectedLGF(const std::string& path, bool skipNeg = true)
{
    using Digraph = lemon::ListDigraph;

    Digraph g;
    Digraph::ArcMap<double> w(g);
    Digraph::NodeMap<int>   nid(g, -1);

    lemon::digraphReader(g, path)
        .nodeMap("id", nid)
        .arcMap("weight", w)
        .run(); // זורק lemon::IoError על קובץ חסר/לא תקין

    // מיפוי מזהי-קובץ -> אינדקס קומפקטי 0..N-1
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

        if (u == v) continue;            // בלי לולאות עצמיות
        if (skipNeg && wt < 0) continue; // לדלג על משקלים שליליים אם ביקשו
        adj[u].push_back({ v, wt });       // מכוון: u -> v
    }
    return adj;
}

// שם API "ידידותי" שהטסטים שלך משתמשים בו
inline Adj lgf_to_adj(const std::string& path, bool skipNeg = true) {
    return loadAdjFromDirectedLGF(path, skipNeg);
}
