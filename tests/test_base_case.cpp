// tests/test_base_case.cpp
#include <gtest/gtest.h>
#include <limits>
#include <vector>
#include <algorithm>
#include "sssp/algorithms/types.hpp"
#include "sssp/algorithms/base_case.hpp"
using namespace sssp;

namespace {
    auto index_of = [](int k) -> std::size_t { return static_cast<std::size_t>(k); };
    template <class T> bool contains(const std::vector<T>& v, const T& x) {
        return std::find(v.begin(), v.end(), x) != v.end();
    }
} // namespace

// 1) מקור בודד ללא שכנים — |U0|=1 ≤ k ⇒ U=U0, B'=B
TEST(BaseCase, SingleNodeNoEdges) {
    AdjList<int> adj(1);
    std::vector<double> db(1, std::numeric_limits<double>::infinity());
    db[0] = 0.0;

    auto out = base_case<int>(/*B*/100.0, /*S*/{ 0 }, adj, db, index_of, /*k*/5);

    EXPECT_DOUBLE_EQ(db[0], 0.0);
    ASSERT_EQ(out.U.size(), 1u);
    EXPECT_EQ(out.U[0], 0);
    EXPECT_DOUBLE_EQ(out.Bprime, 100.0); // B' = B כשה-|U0| ≤ k
}

// 2) עצירה על k+1: בונים מצב שבו |U0|=k+1 ⇒ סינון strict ע"פ B'
TEST(BaseCase, StopsAtKPlusOneFiltersMax) {
    // שרשרת 0->1->2->3 (משקלים 1), k=2 ⇒ U0 יכלול 3 צמתים (0,1,2)
    AdjList<int> adj(4);
    adj[0].push_back({ 1,1.0 });
    adj[1].push_back({ 2,1.0 });
    adj[2].push_back({ 3,1.0 });

    std::vector<double> db(4, std::numeric_limits<double>::infinity());
    db[0] = 0.0;

    auto out = base_case<int>(/*B*/1e9, /*S*/{ 0 }, adj, db, index_of, /*k*/2);

    // עקב הרלאקס לפני העצירה, גם d[3] כבר יכול להתעדכן
    EXPECT_DOUBLE_EQ(db[0], 0.0);
    EXPECT_DOUBLE_EQ(db[1], 1.0);
    EXPECT_DOUBLE_EQ(db[2], 2.0);
    EXPECT_DOUBLE_EQ(db[3], 3.0);

    // |U0|=3>k ⇒ B' = max{0,1,2}=2, U = {v: d[v]<2} = {0,1}
    ASSERT_EQ(out.U.size(), 2u);
    EXPECT_TRUE(contains(out.U, 0));
    EXPECT_TRUE(contains(out.U, 1));
    EXPECT_DOUBLE_EQ(out.Bprime, 2.0);
}

// 3) עצירה על חסם B: cand < B בלבד ⇒ קשתות "יקרות" לא מתעדכנות,
//    ואם |U0| ≤ k ⇒ U=U0 ו-B'=B.
TEST(BaseCase, StopsAtBoundB_ThenReturnsU0AndB) {
    AdjList<int> adj(3);
    adj[0].push_back({ 1, 2.0 });
    adj[0].push_back({ 2, 50.0 });

    std::vector<double> db(3, std::numeric_limits<double>::infinity());
    db[0] = 0.0;

    auto out = base_case<int>(/*B*/2.5, /*S*/{ 0 }, adj, db, index_of, /*k*/10);

    EXPECT_DOUBLE_EQ(db[0], 0.0);
    EXPECT_DOUBLE_EQ(db[1], 2.0);
    EXPECT_TRUE(std::isinf(db[2]));   // 50.0 לא קטן מ-B

    // |U0|=2 ≤ k ⇒ U=U0 (כולל 0 ו-1), B'=B
    ASSERT_EQ(out.U.size(), 2u);
    EXPECT_TRUE(contains(out.U, 0));
    EXPECT_TRUE(contains(out.U, 1));
    EXPECT_DOUBLE_EQ(out.Bprime, 2.5);
}

// 4) קשתות מקבילות — נשמר הזול; |U0|=2 ≤ k ⇒ U=U0, B'=B
TEST(BaseCase, ParallelEdgesKeepCheapest_AndReturnU0WhenSmall) {
    AdjList<int> adj(2);
    adj[0].push_back({ 1, 5.0 });
    adj[0].push_back({ 1, 2.0 }); // הזולה

    std::vector<double> db(2, std::numeric_limits<double>::infinity());
    db[0] = 0.0;

    auto out = base_case<int>(/*B*/100.0, /*S*/{ 0 }, adj, db, index_of, /*k*/10);

    EXPECT_DOUBLE_EQ(db[1], 2.0);
    ASSERT_EQ(out.U.size(), 2u);      // U=U0
    EXPECT_DOUBLE_EQ(out.Bprime, 100.0);
}

// 5) שני מסלולים לאותו יעד — בוחר הקצר
TEST(BaseCase, TwoPathsChooseShorter) {
    AdjList<int> adj(4);
    adj[0].push_back({ 1, 2.0 }); adj[1].push_back({ 3, 2.0 }); // 0->1->3 (4)
    adj[0].push_back({ 2, 1.0 }); adj[2].push_back({ 3, 1.0 }); // 0->2->3 (2)

    std::vector<double> db(4, std::numeric_limits<double>::infinity());
    db[0] = 0.0;

    auto out = base_case<int>(/*B*/1e9, /*S*/{ 0 }, adj, db, index_of, /*k*/10);
    (void)out;

    EXPECT_DOUBLE_EQ(db[3], 2.0);
    EXPECT_DOUBLE_EQ(db[2], 1.0);
    EXPECT_DOUBLE_EQ(db[1], 2.0);
}

// 6) רכיב מנותק נשאר ∞
TEST(BaseCase, DisconnectedRemainInfinity) {
    AdjList<int> adj(5);
    adj[0].push_back({ 1, 1.0 }); adj[1].push_back({ 2, 1.0 }); // רכיב 0-1-2
    adj[3].push_back({ 4, 1.0 });                              // רכיב 3-4 מנותק

    std::vector<double> db(5, std::numeric_limits<double>::infinity());
    db[0] = 0.0;

    auto out = base_case<int>(/*B*/1e9, /*S*/{ 0 }, adj, db, index_of, /*k*/10);
    (void)out;

    EXPECT_DOUBLE_EQ(db[0], 0.0);
    EXPECT_DOUBLE_EQ(db[1], 1.0);
    EXPECT_DOUBLE_EQ(db[2], 2.0);
    EXPECT_TRUE(std::isinf(db[3]));
    EXPECT_TRUE(std::isinf(db[4]));
}

// 7) קצה במשקל 0 + יעד רחוק; |U0| קטן ⇒ U=U0 ו-B'=B
TEST(BaseCase, ZeroEdgeAndFarTarget_ReturnsFullU0WhenSmall) {
    AdjList<int> adj(3);
    adj[0].push_back({ 1, 0.0 });
    adj[0].push_back({ 2, 5.0 });

    std::vector<double> db(3, std::numeric_limits<double>::infinity());
    db[0] = 0.0;

    auto out = base_case<int>(/*B*/100.0, /*S*/{ 0 }, adj, db, index_of, /*k*/10);

    EXPECT_DOUBLE_EQ(db[1], 0.0);
    EXPECT_DOUBLE_EQ(db[2], 5.0);
    ASSERT_EQ(out.U.size(), 3u);      // U=U0 (0,1,2)
    EXPECT_DOUBLE_EQ(out.Bprime, 100.0);
}

// 8) גבולות: cand <= db[v] אבל חייב להיות cand < B כדי להיכנס
TEST(BaseCase, BoundIsStrictOnB) {
    AdjList<int> adj(3);
    // שני מסלולים ל-2 עם אותו משקל 2
    adj[0].push_back({ 1, 1.0 });
    adj[1].push_back({ 2, 1.0 });
    adj[0].push_back({ 2, 2.0 });

    // מקרה א': B=2.0 ⇒ cand=2.0 לא עובר (strict) ⇒ d[2] נשאר ∞
    {
        std::vector<double> db(3, std::numeric_limits<double>::infinity());
        db[0] = 0.0;
        auto out = base_case<int>(/*B*/2.0, /*S*/{ 0 }, adj, db, index_of, /*k*/10);
        (void)out;
        EXPECT_TRUE(std::isinf(db[2]));
    }

    // מקרה ב': B=3.0 ⇒ מעדכן ⇒ d[2]=2.0
    {
        std::vector<double> db(3, std::numeric_limits<double>::infinity());
        db[0] = 0.0;
        auto out = base_case<int>(/*B*/3.0, /*S*/{ 0 }, adj, db, index_of, /*k*/10);
        (void)out;
        EXPECT_DOUBLE_EQ(db[2], 2.0);
    }
}

// 9) k גדול מהגרף — אין בעיות; |U0| ≤ k ⇒ U=U0, B'=B
TEST(BaseCase, LargeKNoOverflow_ReturnsU0AndB) {
    AdjList<int> adj(3);
    adj[0].push_back({ 1, 1.0 });
    adj[1].push_back({ 2, 1.0 });

    std::vector<double> db(3, std::numeric_limits<double>::infinity());
    db[0] = 0.0;

    auto out = base_case<int>(/*B*/1e9, /*S*/{ 0 }, adj, db, index_of, /*k*/1000);

    EXPECT_DOUBLE_EQ(db[0], 0.0);
    EXPECT_DOUBLE_EQ(db[1], 1.0);
    EXPECT_DOUBLE_EQ(db[2], 2.0);
    ASSERT_EQ(out.U.size(), 3u);
    EXPECT_TRUE(contains(out.U, 0));
    EXPECT_TRUE(contains(out.U, 1));
    EXPECT_TRUE(contains(out.U, 2));
    EXPECT_DOUBLE_EQ(out.Bprime, 1e9);
}
