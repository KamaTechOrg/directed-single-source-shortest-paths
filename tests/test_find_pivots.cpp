// tests/find_pivots_all_tests.cpp
#include "sssp/algorithms/find_pivots.hpp"
#include <gtest/gtest.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <utility>
#include <functional>
#include <cstddef>
#include <cmath>
#include <algorithm>

namespace test_helpers {

    // === Tight check (אותו ה-tol כמו במימוש) ===
    inline bool tight(double sum, double dv) {
        double diff = std::abs(sum - dv);
        double scale = 1.0 + std::max(std::abs(sum), std::abs(dv));
        return diff <= 1e-12 * scale;
    }

    // === בניית יער "tight" (children + indeg) מתוך adj/db/W בעזרת index_of ===
    template<class Key, class IndexOf>
    void build_forest_and_validate(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        const std::vector<double>& db,
        const std::vector<Key>& W,
        IndexOf index_of,
        std::vector<std::vector<std::size_t>>& children,
        std::vector<int>& indeg)
    {
        const size_t n = adj.size();
        std::vector<char> inW(n, 0);
        for (auto& v : W) inW[index_of(v)] = 1;

        std::vector<size_t> pred_idx(n, (size_t)-1);
        std::vector<char>   pred_set(n, 0);

        for (auto& uKey : W) {
            size_t iu = index_of(uKey);
            double du = db[iu];
            for (auto& e : adj[iu]) {
                const Key& vKey = e.first;
                double wuv = e.second;
                size_t iv = index_of(vKey);
                if (!inW[iv]) continue;
                double cand = du + wuv;
                if (tight(cand, db[iv])) {
                    if (!pred_set[iv] || iu < pred_idx[iv]) {
                        pred_set[iv] = 1;
                        pred_idx[iv] = iu;
                    }
                }
            }
        }

        children.assign(n, {});
        indeg.assign(n, 0);
        for (auto& vKey : W) {
            size_t iv = index_of(vKey);
            size_t iu = pred_idx[iv];
            if (iu != (size_t)-1) {
                ASSERT_TRUE(inW[iu]) << "parent must be inside W";
                children[iu].push_back(iv);
                ++indeg[iv];
            }
        }

        // לכל v ב-W לכל היותר הורה אחד
        for (auto& vKey : W) {
            size_t iv = index_of(vKey);
            ASSERT_LE(indeg[iv], 1);
        }
    }

    // === ספירת גודל תת-עץ עם עצירה מוקדמת ב-K ===
    inline bool subtree_at_least_K(std::size_t root_idx,
        const std::vector<std::vector<std::size_t>>& children,
        std::size_t K)
    {
        size_t cnt = 0;
        std::vector<std::size_t> st{ root_idx };
        while (!st.empty() && cnt < K) {
            auto x = st.back(); st.pop_back();
            ++cnt;
            for (auto y : children[x]) {
                st.push_back(y);
                if (cnt >= K) break;
            }
        }
        return cnt >= K;
    }

} // namespace test_helpers

// ===========================
// אינדקסרים
// ===========================
#include <type_traits>
#include "sssp/algorithms/relax.hpp" // בשביל החתימות בלבד (לא חובה כאן)
namespace sssp {
    namespace util {
        template<class Key>
        inline auto make_integral_indexer() {
            static_assert(std::is_integral_v<Key>, "Integral keys only");
            if constexpr (std::is_same_v<Key, char>) {
                return [](char c) -> std::size_t {
                    return static_cast<std::size_t>(static_cast<unsigned char>(c));
                    };
            }
            else {
                return [](Key k) -> std::size_t { return static_cast<std::size_t>(k); };
            }
        }
        template<class Key>
        inline auto make_map_indexer(const std::unordered_map<Key, std::size_t>& m) {
            return [&m](const Key& k) -> std::size_t { return m.at(k); };
        }
    }
} // namespace sssp::util

// ===========================
// Tests
// ===========================

// 1) גרף קטן (int), בדיקה גמישה: או Early-Exit (P=S) או פיבוט רוט עם תת-עץ ≥ K
TEST(FindPivots, SmallGraph_IntKeys_Flexible) {
    std::vector<std::vector<std::pair<int, double>>> adj(4);
    adj[0] = { {1,1.0}, {2,2.0} };
    adj[1] = { {2,1.0}, {3,1.0} };
    adj[2] = {};
    adj[3] = {};

    std::vector<double> db = { 0.0, 1e18, 1e18, 1e18 };
    std::vector<int> S = { 0 };
    double B = 1e18;
    std::size_t K = 2;

    auto index_of = sssp::util::make_integral_indexer<int>();
    auto res = sssp::find_pivots<int>(adj, db, S, B, K, index_of);

    ASSERT_FALSE(res.W.empty());
    // אם הופעל Early-Exit: P==S
    if (res.W.size() > K * S.size()) {
        ASSERT_EQ(res.P.size(), S.size());
        EXPECT_EQ(res.P[0], 0);
    }
    else {
        // אחרת – נבדוק רוט ותת-עץ
        std::vector<std::vector<std::size_t>> children;
        std::vector<int> indeg;
        test_helpers::build_forest_and_validate<int>(adj, db, res.W, index_of, children, indeg);
        ASSERT_EQ(indeg[0], 0);
        ASSERT_TRUE(test_helpers::subtree_at_least_K(0, children, K));
        ASSERT_EQ(res.P.size(), 1u);
        EXPECT_EQ(res.P[0], 0);
    }
}

// 2) Early-exit: אם |W| > K|S| → P=S
TEST(FindPivots, EarlyExit_When_W_TooBig) {
    const int n = 6;
    std::vector<std::vector<std::pair<int, double>>> adj(n);
    for (int v = 1; v < n; ++v) adj[0].push_back({ v, 1.0 }); // star מ-0

    std::vector<double> db(n, 1e18);
    db[0] = 0.0;

    std::vector<int> S = { 0 };
    double B = 1e18;
    std::size_t K = 1; // קטן → W גדל מהר

    auto index_of = sssp::util::make_integral_indexer<int>();
    auto res = sssp::find_pivots<int>(adj, db, S, B, K, index_of);

    ASSERT_EQ(res.P.size(), S.size());
    ASSERT_EQ(res.P[0], 0);
    ASSERT_EQ(res.W.size(), static_cast<size_t>(n));
}

// 3) String keys עם index_of
TEST(FindPivots, StringKeys_WithIndexOf) {
    std::vector<std::string> nodes = { "A","B","C","D" };
    std::unordered_map<std::string, std::size_t> idx;
    for (size_t i = 0; i < nodes.size(); ++i) idx[nodes[i]] = i;
    auto index_of = sssp::util::make_map_indexer(idx);

    std::vector<std::vector<std::pair<std::string, double>>> adj(4);
    adj[idx["A"]] = { {"B",1.0}, {"C",2.0} };
    adj[idx["B"]] = { {"C",1.0}, {"D",1.0} };
    adj[idx["C"]] = {};
    adj[idx["D"]] = {};

    std::vector<double> db(4, 1e18);
    db[idx["A"]] = 0.0;

    std::vector<std::string> S = { "A" };
    double B = 1e18;
    std::size_t K = 2;

    auto res = sssp::find_pivots<std::string>(adj, db, S, B, K, index_of);

    ASSERT_FALSE(res.W.empty());
    if (res.W.size() > K * S.size()) {
        ASSERT_EQ(res.P.size(), S.size());
        EXPECT_EQ(res.P[0], "A");
    }
    else {
        std::vector<std::vector<std::size_t>> children;
        std::vector<int> indeg;
        test_helpers::build_forest_and_validate<std::string>(adj, db, res.W, index_of, children, indeg);
        ASSERT_EQ(indeg[idx["A"]], 0);
        ASSERT_TRUE(test_helpers::subtree_at_least_K(idx["A"], children, K));
        ASSERT_EQ(res.P.size(), 1u);
        EXPECT_EQ(res.P[0], "A");
    }
}

// 4) K=1: כל רוט ב-S שמופיע ב-W כשיר (או Early-Exit)
TEST(FindPivots, KEqualsOne_Basic) {
    std::vector<std::vector<std::pair<int, double>>> adj(3);
    adj[0] = { {1,1.0} };
    adj[1] = { {2,1.0} };
    adj[2] = {};

    std::vector<double> db = { 0.0, 1e18, 1e18 };
    std::vector<int> S = { 0 };
    double B = 1e18;
    std::size_t K = 1;

    auto index_of = sssp::util::make_integral_indexer<int>();
    auto res = sssp::find_pivots<int>(adj, db, S, B, K, index_of);

    ASSERT_FALSE(res.W.empty());
    ASSERT_FALSE(res.P.empty());
    EXPECT_EQ(res.P[0], 0);
}

// 5) S ריק: מצפים ל-P ו-W ריקים
TEST(FindPivots, EmptyS_ReturnsEmpty) {
    std::vector<std::vector<std::pair<int, double>>> adj(3);
    adj[0] = { {1,1.0} };
    adj[1] = { {2,1.0} };
    adj[2] = {};

    std::vector<double> db = { 0.0, 1e18, 1e18 };
    std::vector<int> S = {}; // empty
    double B = 1e18;
    std::size_t K = 2;

    auto index_of = sssp::util::make_integral_indexer<int>();
    auto res = sssp::find_pivots<int>(adj, db, S, B, K, index_of);

    EXPECT_TRUE(res.W.empty());
    EXPECT_TRUE(res.P.empty());
}

// 6) אין קשתות "tight" בתוך W: P ⊆ S (או Early-Exit → P=S)
TEST(FindPivots, NoTightEdges_InsideW) {
    std::vector<std::vector<std::pair<int, double>>> adj(3);
    adj[0] = { {1,1.1} };
    adj[1] = { {2,1.1} };
    adj[2] = {};

    std::vector<double> db = { 0.0, 1e18, 1e18 };
    std::vector<int> S = { 0 };
    double B = 1e18;
    std::size_t K = 2;

    auto index_of = sssp::util::make_integral_indexer<int>();
    auto res = sssp::find_pivots<int>(adj, db, S, B, K, index_of);

    for (auto u : res.P) {
        bool inS = (std::find(S.begin(), S.end(), u) != S.end());
        ASSERT_TRUE(inS);
    }
}

// 7) הכרעת שוויון: כשיש שני הורים tight לאותו v, בוחרים את ההורה בעל האינדקס הקטן יותר
TEST(FindPivots, TieBreak_SmallerParentIndexWins) {
    std::vector<std::vector<std::pair<int, double>>> adj(3);
    adj[0] = { {2,2.0} };
    adj[1] = { {2,2.0} };
    adj[2] = {};

    std::vector<double> db = { 0.0, 0.0, 1e18 }; // שני מקורות
    std::vector<int> S = { 0,1 };
    double B = 1e18;
    std::size_t K = 2;

    auto index_of = sssp::util::make_integral_indexer<int>();
    auto res = sssp::find_pivots<int>(adj, db, S, B, K, index_of);

    // בגרף קטן זה לא אמור להפעיל Early-Exit (|W|=3 ≤ 4)
    std::vector<std::vector<std::size_t>> children;
    std::vector<int> indeg;
    test_helpers::build_forest_and_validate<int>(adj, db, res.W, index_of, children, indeg);

    int parent_of_2 = -1;
    for (size_t u = 0; u < children.size(); ++u) {
        for (auto v : children[u]) if (v == 2) parent_of_2 = static_cast<int>(u);
    }
    ASSERT_NE(parent_of_2, -1);
    EXPECT_EQ(parent_of_2, 0); // ההורה בעל אינדקס קטן יותר
}
