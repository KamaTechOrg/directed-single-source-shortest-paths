// tests/test_dijkstra.cpp
#include <gtest/gtest.h>

#include <lemon/list_graph.h>
#include <lemon/dijkstra.h>

using namespace lemon;

TEST(Dijkstra, SmallTriangle) {
    ListDigraph g;

    // Create nodes
    ListDigraph::Node s = g.addNode();
    ListDigraph::Node a = g.addNode();
    ListDigraph::Node b = g.addNode();

    // Create arcs (directed edges)
    ListDigraph::Arc sa = g.addArc(s, a);
    ListDigraph::Arc ab = g.addArc(a, b);
    ListDigraph::Arc sb = g.addArc(s, b);

    // Define weights for the arcs
    ListDigraph::ArcMap<double> w(g);
    w[sa] = 1.0;
    w[ab] = 1.0;
    w[sb] = 3.5;

    // Run Dijkstra from source node s
    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.run(s);

    // Distance checks
    EXPECT_DOUBLE_EQ(d.dist(s), 0.0);        // distance from source to itself
    EXPECT_NEAR(d.dist(a), 1.0, 1e-9);       // shortest path s->a
    EXPECT_NEAR(d.dist(b), 2.0, 1e-9);       // s->a->b is cheaper than s->b direct
}

TEST(Dijkstra, Disconnected) {
    ListDigraph g;

    // Two disconnected components: (s -> a) and (x -> y)
    ListDigraph::Node s = g.addNode();
    ListDigraph::Node a = g.addNode();
    ListDigraph::Node x = g.addNode();
    ListDigraph::Node y = g.addNode();

    ListDigraph::Arc sa = g.addArc(s, a);
    ListDigraph::Arc xy = g.addArc(x, y);

    // Define weights
    ListDigraph::ArcMap<double> w(g);
    w[sa] = 2.0;
    w[xy] = 5.0;

    // Run Dijkstra from source node s
    Dijkstra<ListDigraph, ListDigraph::ArcMap<double>> d(g, w);
    d.run(s);

    EXPECT_DOUBLE_EQ(d.dist(s), 0.0);
    EXPECT_NEAR(d.dist(a), 2.0, 1e-9);

    // Unreachable nodes should not be marked as reached
    EXPECT_FALSE(d.reached(x));
    EXPECT_FALSE(d.reached(y));
}
