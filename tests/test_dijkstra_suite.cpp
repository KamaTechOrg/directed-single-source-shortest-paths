// tests/test_dijkstra_suite.cpp
#include <gtest/gtest.h>
#include <lemon/list_graph.h>
#include <lemon/dijkstra.h>
#include <limits>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cmath>  // for std::isinf


using namespace lemon;

namespace {
    constexpr double kEps = 1e-9;  // tolerance for NEAR checks
    constexpr double kTiny = 1e-9;  // tiny weight used in precision test
    constexpr double kTwoTiny = 2e-9;  // sum of two tiny weights
}

static bool eq(double a, double b, double eps = kEps) {
    if (std::isinf(a) || std::isinf(b)) return std::isinf(a) && std::isinf(b);
    return std::fabs(a - b) <= eps * std::max(1.0, std::max(std::fabs(a), std::fabs(b)));
}

TEST(DijkstraSuite, SingleNodeOnly) {
    ListDigraph g;
    auto s = g.addNode();

    ListDigraph::ArcMap<double> w(g);
    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.run(s);

    ASSERT_DOUBLE_EQ(d.dist(s), 0.0);
}

TEST(DijkstraSuite, TwoNodesOneEdge) {
    ListDigraph g;
    auto s = g.addNode();
    auto t = g.addNode();

    auto st = g.addArc(s, t);
    ListDigraph::ArcMap<double> w(g);
    w[st] = 4.0;

    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.run(s);

    ASSERT_DOUBLE_EQ(d.dist(s), 0.0);
    ASSERT_NEAR(d.dist(t), 4.0, kEps);
}

TEST(DijkstraSuite, ParallelEdgesKeepCheapest) {
    ListDigraph g;
    auto s = g.addNode();
    auto t = g.addNode();

    auto e1 = g.addArc(s, t);
    auto e2 = g.addArc(s, t);
    ListDigraph::ArcMap<double> w(g);
    w[e1] = 10.0;
    w[e2] = 2.5;   // cheaper parallel edge

    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.run(s);

    ASSERT_NEAR(d.dist(t), 2.5, kEps);
}

TEST(DijkstraSuite, SelfLoopDoesNotHelp) {
    ListDigraph g;
    auto s = g.addNode();
    auto a = g.addNode();
    auto sa = g.addArc(s, a);
    auto ss = g.addArc(s, s); // self-loop

    ListDigraph::ArcMap<double> w(g);
    w[sa] = 3.0;
    w[ss] = 0.5;

    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.run(s);

    ASSERT_DOUBLE_EQ(d.dist(s), 0.0);
    ASSERT_NEAR(d.dist(a), 3.0, kEps); // self-loop should not reduce distance to 'a'
}

TEST(DijkstraSuite, ZeroWeightsAndTies) {
    ListDigraph g;
    auto s = g.addNode();
    auto a = g.addNode();
    auto b = g.addNode();

    auto sa = g.addArc(s, a);
    auto ab = g.addArc(a, b);
    auto sb = g.addArc(s, b);

    ListDigraph::ArcMap<double> w(g);
    w[sa] = 0.0;
    w[ab] = 0.0;
    w[sb] = 1.0;

    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.run(s);

    ASSERT_NEAR(d.dist(a), 0.0, kEps);
    ASSERT_NEAR(d.dist(b), 0.0, kEps); // path s->a->b with total 0
}

TEST(DijkstraSuite, SmallCycle) {
    // 0->1 (2), 1->2 (2), 2->0 (2), plus 0->2 (10)
    ListDigraph g;
    auto n0 = g.addNode();
    auto n1 = g.addNode();
    auto n2 = g.addNode();

    auto a01 = g.addArc(n0, n1);
    auto a12 = g.addArc(n1, n2);
    auto a20 = g.addArc(n2, n0);
    auto a02 = g.addArc(n0, n2);

    ListDigraph::ArcMap<double> w(g);
    w[a01] = 2.0; w[a12] = 2.0; w[a20] = 2.0; w[a02] = 10.0;

    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.run(n0);

    ASSERT_NEAR(d.dist(n1), 2.0, kEps);
    ASSERT_NEAR(d.dist(n2), 4.0, kEps); // 0->1->2 is cheaper than 0->2 direct
}

TEST(DijkstraSuite, DisconnectedComponent) {
    ListDigraph g;
    auto s = g.addNode();
    auto a = g.addNode();
    auto x = g.addNode(); // other component
    auto y = g.addNode();

    auto sa = g.addArc(s, a);
    auto xy = g.addArc(x, y);

    ListDigraph::ArcMap<double> w(g);
    w[sa] = 2.0;
    w[xy] = 5.0;

    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.run(s);

    ASSERT_DOUBLE_EQ(d.dist(s), 0.0);
    ASSERT_NEAR(d.dist(a), 2.0, kEps);
    ASSERT_FALSE(d.reached(x));
    ASSERT_FALSE(d.reached(y));
}

TEST(DijkstraSuite, PathReconstructionWithPredMap) {
    // s(0) -> a(1) -> b(2) with costs 1 and 1; also direct s->b with 3.5
    ListDigraph g;
    auto s = g.addNode();
    auto a = g.addNode();
    auto b = g.addNode();

    auto sa = g.addArc(s, a);
    auto ab = g.addArc(a, b);
    auto sb = g.addArc(s, b);

    ListDigraph::ArcMap<double> w(g);
    w[sa] = 1.0; w[ab] = 1.0; w[sb] = 3.5;

    // predecessor map must be NodeMap<Arc>
    ListDigraph::NodeMap<ListDigraph::Arc> predMap(g);

    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.predMap(predMap).run(s);

    // Reconstruct path s->a->b by following predecessors from target 'b'
    std::vector<ListDigraph::Node> path;
    for (ListDigraph::Node v = b; v != INVALID; ) {
        path.push_back(v);
        if (v == s) break;
        ListDigraph::Arc p = predMap[v];
        ASSERT_NE(p, INVALID) << "No predecessor for a non-source node";
        v = g.source(p);
    }
    ASSERT_EQ(path.size(), 3u);
    ASSERT_EQ(path[0], b);
    ASSERT_EQ(path[1], a);
    ASSERT_EQ(path[2], s);
    ASSERT_NEAR(d.dist(b), 2.0, kEps);
}

TEST(DijkstraSuite, FloatingWeightsPrecision) {
    ListDigraph g;
    auto s = g.addNode();
    auto a = g.addNode();
    auto b = g.addNode();

    auto sa = g.addArc(s, a);
    auto ab = g.addArc(a, b);

    ListDigraph::ArcMap<double> w(g);
    w[sa] = kTiny;
    w[ab] = kTiny;

    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.run(s);

    ASSERT_TRUE(eq(d.dist(b), kTwoTiny)); // tolerant comparison
}
