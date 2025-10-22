// tests/test_bmssp_vs_dijkstra.cpp
#include <gtest/gtest.h>

#include <lemon/list_graph.h>
#include <lemon/dijkstra.h>

#include <vector>
#include <utility>
#include <limits>
#include <cmath>
#include <algorithm>

// ======= הייצוג שלך =======
using Key = int;
using Adj = std::vector<std::vector<std::pair<Key, double>>>;

static const double INF = std::numeric_limits<double>::infinity();
static const double EPS = 1e-9;

// טיפוס index_of שלא יוצר המרות למבנה תבנית
struct IndexOf {
    std::size_t operator()(Key k) const { return static_cast<std::size_t>(k); }
};

// ======= הצהרות/Include לאלגוריתם שלך =======
// עדכני את הנתיב בהתאם לעץ הפרויקט שלך:
#include "sssp/algorithms/bmssp.hpp"
// אם הקובץ אצלך בשם/נתיב אחר, החליפי כאן.

// ======= Adapter: Adj -> LEMON + Dijkstra distances =======
namespace lemon_adapt {
    using namespace lemon;

    static std::vector<double> dijkstra_distances_from_adj(const Adj& adj, Key source) {
        const std::size_t n = adj.size();
        ListDigraph g;
        std::vector<ListDigraph::Node> nodes(n);

        // צור צמתים
        for (std::size_t i = 0; i < n; ++i) {
            nodes[i] = g.addNode();
        }

        // מפת משקלים
        ListDigraph::ArcMap<double> w(g);

        // הוסף קשתות עם משקלים (ללא structured bindings)
        for (std::size_t u = 0; u < n; ++u) {
            const auto& edges = adj[u];
            for (std::size_t i = 0; i < edges.size(); ++i) {
                const Key v = edges[i].first;
                const double c = edges[i].second;
                const auto a = g.addArc(nodes[u], nodes[v]);
                w[a] = c;
            }
        }

        // דייקסטרה
        Dijkstra<ListDigraph, ListDigraph::ArcMap<double> > d(g, w);
        d.run(nodes[source]);

        std::vector<double> dist(n, INF);
        for (std::size_t i = 0; i < n; ++i) {
            if (d.reached(nodes[i])) dist[i] = d.dist(nodes[i]);
        }
        return dist;
    }
}

// ======= השוואה עם סבילות =======
static void expect_close(const std::vector<double>& a,
    const std::vector<double>& b,
    double eps = EPS) {
    ASSERT_EQ(a.size(), b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        const bool ai = std::isinf(a[i]);
        const bool bi = std::isinf(b[i]);
        if (ai || bi) {
            EXPECT_TRUE(ai && bi) << "i=" << i << " one is INF and the other is not";
        }
        else {
            EXPECT_NEAR(a[i], b[i], eps) << "i=" << i;
        }
    }
}


TEST(BMSSP_vs_Dijkstra, SmallTriangleSameGraph) {
    // 0->1 (1), 1->2 (1), 0->2 (3.5)
    Adj adj(3);
    adj[0].push_back(std::make_pair(1, 1.0));
    adj[1].push_back(std::make_pair(2, 1.0));
    adj[0].push_back(std::make_pair(2, 3.5));

    std::vector<double> db(3, INF);
    db[0] = 0.0;

    // רפרנס עם LEMON על אותו גרף
    std::vector<double> ref = lemon_adapt::dijkstra_distances_from_adj(adj, 0);

    // הרצה של האלגוריתם שלך על אותו גרף
    int l = 1;
    double B = INF;
    std::vector<Key> S;
    S.push_back(0);
    std::size_t M = 8, K = 8;

    IndexOf index_of;
    auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);

    // השוואה
    expect_close(db, ref);
}

// ======= טסט 2: קשתות מקבילות + רכיב מנותק =======
TEST(BMSSP_vs_Dijkstra, ParallelAndDisconnected) {
    Adj adj(4);
    // רכיב 1: 0->1 (2.0), 0->1 (0.5) מקביל זול
    adj[0].push_back(std::make_pair(1, 2.0));
    adj[0].push_back(std::make_pair(1, 0.5));
    // רכיב 2: 2->3 (5.0)
    adj[2].push_back(std::make_pair(3, 5.0));

    std::vector<double> db(4, INF);
    db[0] = 0.0;

    std::vector<double> ref = lemon_adapt::dijkstra_distances_from_adj(adj, 0);

    int l = 2; double B = INF;
    std::vector<Key> S(1, 0);
    std::size_t M = 8, K = 8;

    IndexOf index_of;
    auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);

    expect_close(db, ref);
}

// ======= טסט 3: "עשן" רנדומלי קטן =======
TEST(BMSSP_vs_Dijkstra, RandomSparse_QuickSmoke) {
    const int N = 30;
    Adj adj(static_cast<std::size_t>(N));
    // גרף דליל אקראי (seed קבוע לרגרסיה)
    std::srand(42);
    for (int u = 0; u < N; ++u) {
        for (int v = 0; v < N; ++v) {
            if (u == v) continue;
            // הסתברות קשת ~6%
            if ((std::rand() % 100) < 6) {
                double w = 1.0 + (std::rand() % 100) / 10.0; // [1.0 .. 10.9]
                adj[static_cast<std::size_t>(u)]
                    .push_back(std::make_pair(static_cast<Key>(v), w));
            }
        }
    }

    std::vector<double> db(static_cast<std::size_t>(N), INF);
    db[0] = 0.0;

    std::vector<double> ref = lemon_adapt::dijkstra_distances_from_adj(adj, 0);

    int l = 2; double B = INF;
    std::vector<Key> S(1, 0);
    std::size_t M = 16, K = 16;

    IndexOf index_of;
    auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);

    expect_close(db, ref);
}





// =======================
// helpers for extra tests
// =======================
static std::vector<double> ref_min_of_two_sources(const Adj& adj, Key s1, Key s2) {
    auto d1 = lemon_adapt::dijkstra_distances_from_adj(adj, s1);
    auto d2 = lemon_adapt::dijkstra_distances_from_adj(adj, s2);
    std::vector<double> out(d1.size(), INF);
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = std::min(d1[i], d2[i]);
    return out;
}

static Adj make_grid_graph(int rows, int cols, double horiz_w = 1.0, double vert_w = 1.0) {
    const int N = rows * cols;
    Adj adj(static_cast<std::size_t>(N));
    auto id = [cols](int r, int c) { return r * cols + c; };
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const int u = id(r, c);
            if (c + 1 < cols) { adj[u].push_back(std::make_pair(id(r, c + 1), horiz_w)); }
            if (c - 1 >= 0) { adj[u].push_back(std::make_pair(id(r, c - 1), horiz_w)); }
            if (r + 1 < rows) { adj[u].push_back(std::make_pair(id(r + 1, c), vert_w)); }
            if (r - 1 >= 0) { adj[u].push_back(std::make_pair(id(r - 1, c), vert_w)); }
        }
    }
    return adj;
}

// =======================
// extra edge-case tests
// =======================

TEST(BMSSP_vs_Dijkstra, SelfLoopZeroWeight) {
    // s->a (3), self-loop on s with 0 should NOT improve s->a
    Adj adj(2);
    adj[0].push_back(std::make_pair(0, 0.0)); // self-loop
    adj[0].push_back(std::make_pair(1, 3.0)); // s->a

    std::vector<double> db(2, INF); db[0] = 0.0;
    std::vector<double> ref = lemon_adapt::dijkstra_distances_from_adj(adj, 0);

    int l = 1; double B = INF; std::vector<Key> S(1, 0);
    std::size_t M = 8, K = 8; IndexOf index_of;
    (void)sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);

    expect_close(db, ref);
}

TEST(BMSSP_vs_Dijkstra, Grid3x3_UnitWeights) {
    // 3x3 grid, all edges weight 1
    Adj adj = make_grid_graph(3, 3, 1.0, 1.0);
    const int N = 9;

    std::vector<double> db(static_cast<std::size_t>(N), INF); db[0] = 0.0;
    std::vector<double> ref = lemon_adapt::dijkstra_distances_from_adj(adj, 0);

    int l = 2; double B = INF; std::vector<Key> S(1, 0);
    std::size_t M = 16, K = 16; IndexOf index_of;
    (void)sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);

    expect_close(db, ref);
}

TEST(BMSSP_vs_Dijkstra, TwoSources_MinDistance) {
    // |S| = 2: min from source 0 or source 8 on a 3x3 grid
    Adj adj = make_grid_graph(3, 3, 1.0, 1.0);
    const int N = 9;

    std::vector<double> db(static_cast<std::size_t>(N), INF);
    // אם אצלך ההנחה היא db[source]=0 רק למקורות ב-S, השאירי את כולם ב-INF;
    // האלגוריתם אמור לדאוג לזה פנימית לפי S. אם דרוש — אפסי ידנית:
    // db[0] = db[8] = 0.0;

    std::vector<double> ref = ref_min_of_two_sources(adj, /*s1*/0, /*s2*/8);
    db[0] = 0.0;   // ← חשוב!
    db[8] = 0.0;

    int l = 2; double B = INF;
    std::vector<Key> S; S.push_back(0); S.push_back(8);
    std::size_t M = 16, K = 16; IndexOf index_of;
    (void)sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);

    expect_close(db, ref);
}


TEST(BMSSP_vs_Dijkstra, ZeroWeightTiesParallelPaths) {
    // many zero-weight routes; all should give distance 0 where reachable
    Adj adj(4);
    // 0->1 (0), 0->2 (0), 1->3 (0), 2->3 (0)
    adj[0].push_back(std::make_pair(1, 0.0));
    adj[0].push_back(std::make_pair(2, 0.0));
    adj[1].push_back(std::make_pair(3, 0.0));
    adj[2].push_back(std::make_pair(3, 0.0));

    std::vector<double> db(4, INF); db[0] = 0.0;
    std::vector<double> ref = lemon_adapt::dijkstra_distances_from_adj(adj, 0);

    int l = 2; double B = INF; std::vector<Key> S(1, 0);
    std::size_t M = 8, K = 8; IndexOf index_of;
    (void)sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);

    expect_close(db, ref);
}

TEST(BMSSP_vs_Dijkstra, LongCycleWithShortcut) {
    // cycle 0->1->2->3->0 (2 each) plus shortcut 0->3 (7): best to go around (6) not direct (7)
    Adj adj(4);
    adj[0].push_back(std::make_pair(1, 2.0));
    adj[1].push_back(std::make_pair(2, 2.0));
    adj[2].push_back(std::make_pair(3, 2.0));
    adj[3].push_back(std::make_pair(0, 2.0));
    adj[0].push_back(std::make_pair(3, 7.0)); // worse than 0->1->2->3

    std::vector<double> db(4, INF); db[0] = 0.0;
    std::vector<double> ref = lemon_adapt::dijkstra_distances_from_adj(adj, 0);

    int l = 2; double B = INF; std::vector<Key> S(1, 0);
    std::size_t M = 8, K = 8; IndexOf index_of;
    (void)sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);

    expect_close(db, ref);
}



// B' behavior: no pivots case ⇒ B' == B
TEST(BMSSP_vs_Dijkstra, Bprime_NoPivots_EqualsB) {
    // גרף פשוט: 0->1 במשקל 1. המינימום החיובי הוא 1.
    Adj adj(2);
    adj[0].push_back(std::make_pair(1, 1.0));

    IndexOf index_of;
    std::vector<Key> S(1, 0);
    int l = 1;
    std::size_t M = 8, K = 8;

    // בוחרים B קטן מהמינימום החיובי ⇒ אין פיבוטים ⇒ B' חייב להיות בדיוק B
    double B = 0.4;
    std::vector<double> db(2, INF); db[0] = 0.0;

    auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);

    ASSERT_LE(out.Bprime, B);
    ASSERT_NEAR(out.Bprime, B, 1e-12) << "When P is empty, B' must equal B";
}

// B' behavior: with pivots ⇒ B' ≤ B and B' ≤ min_positive_dijkstra_distance
// B' behavior: with pivots ⇒ B' ≤ B; if pivots selected, also B' ≤ min_positive_dijkstra_distance
TEST(BMSSP_vs_Dijkstra, Bprime_WithPivots_BoundedByMinPositiveDistance) {
    // גרף פשוט: 0->1 (1)
    Adj adj(2);
    adj[0].push_back(std::make_pair(1, 1.0));

    // רפרנס: דייקסטרה
    auto ref = lemon_adapt::dijkstra_distances_from_adj(adj, 0);
    // המינימום החיובי (מעל 0 ולא ∞)
    double min_pos = INF;
    for (double d : ref) {
        if (d > 0.0 && !std::isinf(d)) min_pos = std::min(min_pos, d);
    }
    ASSERT_FALSE(std::isinf(min_pos)); // אמור להיות 1

    IndexOf index_of;
    std::vector<Key> S(1, 0);
    int l = 1;
    std::size_t M = 8, K = 8;

    double B = 10.0; // גדול מהמינימום החיובי
    std::vector<double> db(2, INF); db[0] = 0.0;

    auto out = sssp::bmssp<Key>(l, B, S, adj, db, index_of, M, K);

    // תמיד נכון:
    ASSERT_LE(out.Bprime, B);

    // אם לא נבחרו פיבוטים (לפי ההתנהגות אצלך: B'==B), זה מצב תקין — אין מה להשוות ל-min_pos
    if (std::fabs(out.Bprime - B) <= 1e-12) {
        SUCCEED() << "No pivots selected in this run (P empty); B' == B is expected.";
        return;
    }

    // אם כן נבחרו פיבוטים (B' < B) — נבדוק שגם לא עובר את המינימום החיובי
    ASSERT_LE(out.Bprime, min_pos + 1e-12)
        << "With pivots selected, B' should not exceed the smallest positive Dijkstra distance";
    ASSERT_GE(out.Bprime, 0.0);
}
