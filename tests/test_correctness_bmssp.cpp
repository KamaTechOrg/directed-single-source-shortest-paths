// tests/test_correctness_bmssp.cpp
#include <gtest/gtest.h>
#include <vector>
#include <utility>
#include <random>
#include <limits>
#include <cmath>
#include <algorithm>

#include <lemon/list_graph.h>
#include <lemon/dijkstra.h>

#include "sssp/algorithms/bmssp.hpp"

// ===== Types =====
using Key = int;
using Adj = std::vector<std::vector<std::pair<Key, double>>>;
static const double INF = std::numeric_limits<double>::infinity();

// ===== Build a LEMON graph in parallel to Adj =====
struct LemonPack {
    lemon::ListDigraph g;
    lemon::ListDigraph::ArcMap<double> w;
    lemon::ListDigraph::Node src;
    std::vector<lemon::ListDigraph::Node> nodes;

    LemonPack() : w(g) {}
    LemonPack(const LemonPack&) = delete;
    LemonPack& operator=(const LemonPack&) = delete;
};

static void make_lemon_from_adj(const Adj& adj, int source, LemonPack& P) {
    const std::size_t n = adj.size();

    // במקום "P = LemonPack();" שמבצע השמה אסורה — ננקה את הקיים:
    P.g.clear();
    P.nodes.clear();
    P.nodes.reserve(n);

    // לבנות צמתים
    for (std::size_t i = 0; i < n; ++i) {
        P.nodes.push_back(P.g.addNode());
    }
    P.src = P.nodes.at(static_cast<std::size_t>(source));

    // לבנות קשתות + משקלים
    for (std::size_t u = 0; u < n; ++u) {
        const auto& edges = adj[u];
        for (std::size_t ei = 0; ei < edges.size(); ++ei) {
            Key v = edges[ei].first;
            double wt = edges[ei].second;
            if (u == static_cast<std::size_t>(v)) continue;
            auto a = P.g.addArc(P.nodes[u], P.nodes[static_cast<std::size_t>(v)]);
            P.w[a] = wt;
        }
    }
}

static std::vector<double> run_dijkstra(const LemonPack& P) {
    std::vector<double> d(lemon::countNodes(P.g), INF);
    lemon::Dijkstra<lemon::ListDigraph, decltype(P.w)> dij(P.g, P.w);
    dij.run(P.src);
    for (std::size_t i = 0; i < d.size(); ++i) {
        if (dij.reached(P.nodes[i])) d[i] = dij.dist(P.nodes[i]);
    }
    return d;
}

static void check_triangle_inequality(const Adj& adj, const std::vector<double>& d) {
    const std::size_t n = adj.size();
    for (std::size_t u = 0; u < n; ++u) {
        if (!std::isfinite(d[u])) continue;
        const auto& edges = adj[u];
        for (std::size_t ei = 0; ei < edges.size(); ++ei) {
            Key v = edges[ei].first;
            double wt = edges[ei].second;
            ASSERT_LE(d[static_cast<std::size_t>(v)], d[u] + wt + 1e-12)
                << "Triangle violated at " << u << "->" << v;
        }
    }
}

// ===== BMSSP runner =====
template<class IndexOf>
static std::vector<double> run_bmssp(const Adj& adj, int source, IndexOf index_of) {
    const std::size_t n = adj.size();
    std::vector<double> db(n, INF);
    db[static_cast<std::size_t>(source)] = 0.0;

    std::vector<Key> S = { source };
    int l = 2;
    double B = INF;
    std::size_t M = 1u << 16;
    std::size_t K = 1u << 16;
    auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);
    (void)out;
    return db;
}

// ===== Synthetic graph builders =====
static Adj make_line_graph(int n) {
    Adj adj(static_cast<std::size_t>(n));
    for (int i = 0; i < n - 1; ++i) {
        adj[static_cast<std::size_t>(i)].push_back({ i + 1, 1.0 });
    }
    return adj;
}

static Adj make_star_graph(int n) {
    Adj adj(static_cast<std::size_t>(n));
    for (int i = 1; i < n; ++i) {
        adj[0].push_back({ i, 1.0 });
    }
    return adj;
}

static Adj make_disconnected(int m, int singles) {
    Adj adj(static_cast<std::size_t>(m + singles));
    for (int i = 0; i < m - 1; ++i) {
        adj[static_cast<std::size_t>(i)].push_back({ i + 1, 1.0 });
    }
    return adj;
}

static Adj make_random_graph(int n, int m, unsigned seed = 42) {
    Adj adj(static_cast<std::size_t>(n));
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> U(0, n - 1);
    std::uniform_real_distribution<double> Wdist(0.1, 5.0);
    for (int i = 0; i < m; ++i) {
        int u = U(rng), v = U(rng);
        if (u == v) continue;
        adj[static_cast<std::size_t>(u)].push_back({ v, Wdist(rng) });
    }
    return adj;
}

// ===== TESTS =====
TEST(BMSSP_Correctness, LineGraph) {
    Adj adj = make_line_graph(50);
    int s = 0;
    auto idx = [](Key k) { return static_cast<std::size_t>(k); };

    auto db = run_bmssp(adj, s, idx);
    LemonPack P; make_lemon_from_adj(adj, s, P);
    auto ref = run_dijkstra(P);

    ASSERT_EQ(db.size(), ref.size());
    for (std::size_t i = 0; i < db.size(); ++i) {
        if (std::isinf(ref[i])) { ASSERT_TRUE(std::isinf(db[i])); }
        else { ASSERT_NEAR(db[i], ref[i], 1e-9); }
    }
    check_triangle_inequality(adj, db);
}

TEST(BMSSP_Correctness, StarGraph) {
    Adj adj = make_star_graph(200);
    int s = 0;
    auto idx = [](Key k) { return static_cast<std::size_t>(k); };

    auto db = run_bmssp(adj, s, idx);
    LemonPack P; make_lemon_from_adj(adj, s, P);
    auto ref = run_dijkstra(P);

    ASSERT_EQ(db.size(), ref.size());
    for (std::size_t i = 0; i < db.size(); ++i) {
        if (std::isinf(ref[i])) { ASSERT_TRUE(std::isinf(db[i])); }
        else { ASSERT_NEAR(db[i], ref[i], 1e-9); }
    }
    check_triangle_inequality(adj, db);
}

TEST(BMSSP_Correctness, Disconnected) {
    Adj adj = make_disconnected(30, 20);
    int s = 0;
    auto idx = [](Key k) { return static_cast<std::size_t>(k); };

    auto db = run_bmssp(adj, s, idx);
    LemonPack P; make_lemon_from_adj(adj, s, P);
    auto ref = run_dijkstra(P);

    ASSERT_EQ(db.size(), ref.size());
    for (std::size_t i = 0; i < db.size(); ++i) {
        if (std::isinf(ref[i])) { ASSERT_TRUE(std::isinf(db[i])); }
        else { ASSERT_NEAR(db[i], ref[i], 1e-9); }
    }
    check_triangle_inequality(adj, db);
}

TEST(BMSSP_Correctness, RandomPositiveWeights) {
    Adj adj = make_random_graph(500, 3000, /*seed=*/1337);
    int s = 0;
    auto idx = [](Key k) { return static_cast<std::size_t>(k); };

    auto db = run_bmssp(adj, s, idx);
    LemonPack P; make_lemon_from_adj(adj, s, P);
    auto ref = run_dijkstra(P);

    ASSERT_EQ(db.size(), ref.size());
    std::size_t mismatches = 0; double max_abs = 0.0;
    for (std::size_t i = 0; i < db.size(); ++i) {
        bool ainf = std::isinf(db[i]);
        bool binf = std::isinf(ref[i]);
        if (ainf != binf) { ++mismatches; continue; }
        if (!ainf) {
            max_abs = std::max(max_abs, std::abs(db[i] - ref[i]));
            ASSERT_NEAR(db[i], ref[i], 1e-8);
        }
    }
    ASSERT_EQ(mismatches, 0u);
    ASSERT_LE(max_abs, 1e-8);
    check_triangle_inequality(adj, db);
}



// ===== Extra generators for hard cases =====

// גרף צפוף: כמעט כל-אל-כל (למעט לולאות)
static Adj make_dense_graph(int n, double base_w = 1.0) {
    Adj adj(static_cast<std::size_t>(n));
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            if (u == v) continue;
            // משקל משתנה קלות כדי לשבור סימטריה
            double w = base_w + ((u + v) % 7) * 0.01;
            adj[static_cast<std::size_t>(u)].push_back({ v, w });
        }
    }
    return adj;
}

// Grid דו-ממדי בגודל rows x cols, קשתות 4-כיוונים עם משקל 1
static Adj make_grid_2d(int rows, int cols) {
    const int n = rows * cols;
    auto id = [cols](int r, int c) { return r * cols + c; };
    Adj adj(static_cast<std::size_t>(n));
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const int u = id(r, c);
            if (r + 1 < rows) adj[static_cast<std::size_t>(u)].push_back({ id(r + 1,c), 1.0 });
            if (r - 1 >= 0)   adj[static_cast<std::size_t>(u)].push_back({ id(r - 1,c), 1.0 });
            if (c + 1 < cols) adj[static_cast<std::size_t>(u)].push_back({ id(r,c + 1), 1.0 });
            if (c - 1 >= 0)   adj[static_cast<std::size_t>(u)].push_back({ id(r,c - 1), 1.0 });
        }
    }
    return adj;
}

// טסט עומק: קו ארוך מאוד עם משקלים אקראיים חיוביים
static Adj make_long_weighted_line(int n, unsigned seed = 7) {
    Adj adj(static_cast<std::size_t>(n));
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> W(0.01, 3.0);
    for (int i = 0; i < n - 1; ++i) {
        adj[static_cast<std::size_t>(i)].push_back({ i + 1, W(rng) });
    }
    return adj;
}

// טסט קצוות מקבילים: כמה קשתות בין אותו זוג, עם משקל מינימלי שמכריע
static Adj make_parallel_edges_graph() {
    const int n = 10;
    Adj adj(static_cast<std::size_t>(n));
    // 0 -> 1 עם כמה משקלים שונים
    adj[0].push_back({ 1, 5.0 });
    adj[0].push_back({ 1, 2.0 });   // זה אמור להכריע
    adj[0].push_back({ 1, 3.5 });
    // המשך מסלול חד-כיווני
    for (int i = 1; i < n - 1; ++i) adj[static_cast<std::size_t>(i)].push_back({ i + 1, 1.0 });
    return adj;
}

// טסט Self-loops: קשתות u->u שלא אמורות להזיק (אנחנו מדלגים עליהן בבילדר LEMON)
static Adj make_self_loops_graph() {
    const int n = 20;
    Adj adj(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        adj[static_cast<std::size_t>(i)].push_back({ i, 0.5 });        // self-loop
        if (i + 1 < n) adj[static_cast<std::size_t>(i)].push_back({ i + 1, 1.0 });
    }
    return adj;
}

// טסט משקלים שבריים קטנים מאוד (דיוק צף)
static Adj make_small_fractional_weights(int n, int m, unsigned seed = 123) {
    Adj adj(static_cast<std::size_t>(n));
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> U(0, n - 1);
    std::uniform_real_distribution<double> W(1e-6, 1e-3); // משקלים זעירים
    for (int i = 0; i < m; ++i) {
        int u = U(rng), v = U(rng);
        if (u == v) continue;
        adj[static_cast<std::size_t>(u)].push_back({ v, W(rng) });
    }
    return adj;
}

// ===== Extra tests =====

// צפוף: נוודא שאין סטיות גם כשיש המון קשתות (n=120 נותן ~14K קשתות)
TEST(BMSSP_Correctness, DenseGraph) {
    Adj adj = make_dense_graph(120, 1.0);
    int s = 0;
    auto idx = [](Key k) { return static_cast<std::size_t>(k); };

    auto db = run_bmssp(adj, s, idx);
    LemonPack P; make_lemon_from_adj(adj, s, P);
    auto ref = run_dijkstra(P);

    ASSERT_EQ(db.size(), ref.size());
    for (std::size_t i = 0; i < db.size(); ++i) {
        if (std::isinf(ref[i])) { ASSERT_TRUE(std::isinf(db[i])); }
        else { ASSERT_NEAR(db[i], ref[i], 1e-7); }
    }
    check_triangle_inequality(adj, db);
}

// Grid 2D בינוני: עומק גדל בצורה צפויה (מרחקי מנהטן)
TEST(BMSSP_Correctness, Grid2D) {
    Adj adj = make_grid_2d(40, 40); // 1600 צמתים
    int s = 0;
    auto idx = [](Key k) { return static_cast<std::size_t>(k); };

    auto db = run_bmssp(adj, s, idx);
    LemonPack P; make_lemon_from_adj(adj, s, P);
    auto ref = run_dijkstra(P);

    ASSERT_EQ(db.size(), ref.size());
    for (std::size_t i = 0; i < db.size(); ++i) {
        if (std::isinf(ref[i])) { ASSERT_TRUE(std::isinf(db[i])); }
        else { ASSERT_NEAR(db[i], ref[i], 1e-9); }
    }
    check_triangle_inequality(adj, db);
}

// קו ארוך עם משקלים אקראיים: בודק עומק גדול וצבירת שברים
TEST(BMSSP_Correctness, LongWeightedLine) {
    Adj adj = make_long_weighted_line(3000, /*seed=*/7);
    int s = 0;
    auto idx = [](Key k) { return static_cast<std::size_t>(k); };

    auto db = run_bmssp(adj, s, idx);
    LemonPack P; make_lemon_from_adj(adj, s, P);
    auto ref = run_dijkstra(P);

    ASSERT_EQ(db.size(), ref.size());
    for (std::size_t i = 0; i < db.size(); ++i) {
        if (std::isinf(ref[i])) { ASSERT_TRUE(std::isinf(db[i])); }
        else { ASSERT_NEAR(db[i], ref[i], 1e-7); }
    }
    check_triangle_inequality(adj, db);
}

// קצוות מקבילים: נוודא שהמסלול בוחר את הקשת בעלת המשקל המינימלי
TEST(BMSSP_Correctness, ParallelEdges) {
    Adj adj = make_parallel_edges_graph();
    int s = 0;
    auto idx = [](Key k) { return static_cast<std::size_t>(k); };
    auto db = run_bmssp(adj, s, idx);

    LemonPack P; make_lemon_from_adj(adj, s, P);
    auto ref = run_dijkstra(P);

    ASSERT_EQ(db.size(), ref.size());
    for (std::size_t i = 0; i < db.size(); ++i) {
        if (std::isinf(ref[i])) { ASSERT_TRUE(std::isinf(db[i])); }
        else { ASSERT_NEAR(db[i], ref[i], 1e-9); }
    }
    // בדיקה ספציפית: d[1] חייב להיות 2.0 (הקשת הזולה בין המקבילות)
    ASSERT_NEAR(db[1], 2.0, 1e-12);
    check_triangle_inequality(adj, db);
}

// Self-loops: לוודא שלא “שוברים” את האלגוריתם (אנחנו מדלגים על u==v)
TEST(BMSSP_Correctness, SelfLoopsAreIgnored) {
    Adj adj = make_self_loops_graph();
    int s = 0;
    auto idx = [](Key k) { return static_cast<std::size_t>(k); };

    auto db = run_bmssp(adj, s, idx);
    LemonPack P; make_lemon_from_adj(adj, s, P);
    auto ref = run_dijkstra(P);

    ASSERT_EQ(db.size(), ref.size());
    for (std::size_t i = 0; i < db.size(); ++i) {
        if (std::isinf(ref[i])) { ASSERT_TRUE(std::isinf(db[i])); }
        else { ASSERT_NEAR(db[i], ref[i], 1e-9); }
    }
    check_triangle_inequality(adj, db);
}

// משקלים זעירים שבריים: רגישות לדיוק נקודתי
TEST(BMSSP_Correctness, TinyFractionalWeights) {
    Adj adj = make_small_fractional_weights(600, 4000, /*seed=*/123);
    int s = 0;
    auto idx = [](Key k) { return static_cast<std::size_t>(k); };

    auto db = run_bmssp(adj, s, idx);
    LemonPack P; make_lemon_from_adj(adj, s, P);
    auto ref = run_dijkstra(P);

    ASSERT_EQ(db.size(), ref.size());
    // כאן נסבול טיפה יותר רעש מספרי
    for (std::size_t i = 0; i < db.size(); ++i) {
        if (std::isinf(ref[i])) { ASSERT_TRUE(std::isinf(db[i])); }
        else { ASSERT_NEAR(db[i], ref[i], 1e-6); }
    }
    check_triangle_inequality(adj, db);
}
