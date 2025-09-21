// tests/test_relax.cpp
#include <gtest/gtest.h>
#include "sssp/algorithms/relax.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <limits>
#include <set>
#include <algorithm>
#include <cmath>

using std::vector;
using std::pair;
using std::string;

static constexpr double INF = std::numeric_limits<double>::infinity();

// helper: assert that vec is exactly the set 'expected' (no duplicates)
template<class T>
static void expect_set_eq(const std::vector<T>& vec, const std::set<T>& expected) {
    std::set<T> got(vec.begin(), vec.end());
    EXPECT_EQ(got, expected);
    EXPECT_EQ(vec.size(), got.size()); // no duplicates
}

// ---------------------------------------
// Tests for integral Key (int) + index_of
// ---------------------------------------

TEST(RelaxInt, LinearChain_PropagatesUpToK) {
    // 0->1->2->3 with weight 1 on each edge
    vector<vector<pair<int, double>>> adj(4);
    adj[0] = { {1,1.0} };
    adj[1] = { {2,1.0} };
    adj[2] = { {3,1.0} };

    vector<double> db = { 0.0, INF, INF, INF };
    vector<int> S = { 0 };
    double B = INF;
    std::size_t K = 2;

    auto index_of = [](int k) -> std::size_t { return static_cast<std::size_t>(k); };
    auto out = sssp::relax_k_steps<int>(adj, db, std::move(S), B, K, index_of);

    // expected after 2 steps: d1=1, d2=2, d3=INF
    EXPECT_DOUBLE_EQ(db[1], 1.0);
    EXPECT_DOUBLE_EQ(db[2], 2.0);
    EXPECT_TRUE(std::isinf(db[3]));

    // W = {0,1,2}
    expect_set_eq(out.W_union, std::set<int>({ 0,1,2 }));
}

TEST(RelaxInt, StopsEarlyWhenFrontierEmpties) {
    vector<vector<pair<int, double>>> adj(4);
    adj[0] = { {1,1.0} };
    adj[1] = { {2,1.0} };
    adj[2] = { {3,1.0} };

    vector<double> db = { 0.0, INF, INF, INF };
    vector<int> S = { 0 };
    double B = INF;
    std::size_t K = 10;

    auto index_of = [](int k) -> std::size_t { return static_cast<std::size_t>(k); };
    auto out = sssp::relax_k_steps<int>(adj, db, std::move(S), B, K, index_of);

    expect_set_eq(out.W_union, std::set<int>({ 0,1,2,3 }));
    EXPECT_DOUBLE_EQ(db[3], 3.0);
}

TEST(RelaxInt, StrictThresholdB_ExcludesEqualToB) {
    // 0->1 (1), 1->2 (1). B=2 => node 2 not in W (db[2]==B is not < B)
    vector<vector<pair<int, double>>> adj(3);
    adj[0] = { {1,1.0}, {2,2.0} };
    adj[1] = { {2,1.0} };

    vector<double> db = { 0.0, INF, INF };
    vector<int> S = { 0 };
    double B = 2.0;
    std::size_t K = 5;

    auto index_of = [](int k) -> std::size_t { return static_cast<std::size_t>(k); };
    auto out = sssp::relax_k_steps<int>(adj, db, std::move(S), B, K, index_of);

    EXPECT_DOUBLE_EQ(db[1], 1.0);
    EXPECT_DOUBLE_EQ(db[2], 2.0); // == B
    expect_set_eq(out.W_union, std::set<int>({ 0,1 })); // 2 not in W
}

TEST(RelaxInt, KZero_ReturnsW0Only_NoDistanceUpdates) {
    vector<vector<pair<int, double>>> adj(3);
    adj[0] = { {1,1.0} };
    vector<double> db = { 0.0, INF, INF };
    vector<int> S = { 0 };

    auto index_of = [](int k) -> std::size_t { return static_cast<std::size_t>(k); };
    auto out = sssp::relax_k_steps<int>(adj, db, std::move(S), /*B*/INF, /*K*/0, index_of);

    expect_set_eq(out.W_union, std::set<int>({ 0 }));
    EXPECT_TRUE(std::isinf(db[1])); // no updates
}

TEST(RelaxInt, EmptyS_NoProgress) {
    vector<vector<pair<int, double>>> adj(3);
    vector<double> db = { 0.0, INF, INF };
    vector<int> S; // empty

    auto index_of = [](int k) -> std::size_t { return static_cast<std::size_t>(k); };
    auto out = sssp::relax_k_steps<int>(adj, db, std::move(S), /*B*/INF, /*K*/5, index_of);

    expect_set_eq(out.W_union, std::set<int>({}));
    EXPECT_DOUBLE_EQ(db[0], 0.0);
    EXPECT_TRUE(std::isinf(db[1]));
    EXPECT_TRUE(std::isinf(db[2]));
}

TEST(RelaxInt, NegativeWeights_AreApplied) {
    vector<vector<pair<int, double>>> adj(2);
    adj[0] = { {1,-3.0} };

    vector<double> db = { 0.0, INF };
    vector<int> S = { 0 };

    auto index_of = [](int k) -> std::size_t { return static_cast<std::size_t>(k); };
    auto out = sssp::relax_k_steps<int>(adj, db, std::move(S), /*B*/INF, /*K*/1, index_of);

    EXPECT_DOUBLE_EQ(db[1], -3.0);
    expect_set_eq(out.W_union, std::set<int>({ 0,1 }));
}

TEST(RelaxInt, EqualPaths_NoDuplicatesInW) {
    // direct 0->2 with 2; also 0->1->2 with 1+1.
    vector<vector<pair<int, double>>> adj(3);
    adj[0] = { {1,1.0}, {2,2.0} };
    adj[1] = { {2,1.0} };

    vector<double> db = { 0.0, INF, INF };
    vector<int> S = { 0 };

    auto index_of = [](int k) -> std::size_t { return static_cast<std::size_t>(k); };
    auto out = sssp::relax_k_steps<int>(adj, db, std::move(S), /*B*/INF, /*K*/3, index_of);

    EXPECT_DOUBLE_EQ(db[2], 2.0);
    expect_set_eq(out.W_union, std::set<int>({ 0,1,2 }));
}

// -------------------------------------------------
// Tests for std::string keys with custom index_of
// -------------------------------------------------

TEST(RelaxString, BasicChain_WithIndexOf) {
    vector<string> nodes = { "A","B","C" };
    std::unordered_map<string, std::size_t> idx;
    for (std::size_t i = 0; i < nodes.size(); ++i) idx[nodes[i]] = i;
    auto index_of = [&](const string& s) { return idx[s]; };

    vector<vector<pair<string, double>>> adj(nodes.size());
    adj[idx["A"]] = { {"B",1.0} };
    adj[idx["B"]] = { {"C",1.0} };

    vector<double> db(nodes.size(), INF);
    db[idx["A"]] = 0.0;

    vector<string> S = { "A" };
    auto out = sssp::relax_k_steps(adj, db, std::move(S), /*B*/INF, /*K*/2, index_of);

    EXPECT_DOUBLE_EQ(db[idx["B"]], 1.0);
    EXPECT_DOUBLE_EQ(db[idx["C"]], 2.0);
    expect_set_eq(out.W_union, std::set<string>({ "A","B","C" }));
}

TEST(RelaxString, ThresholdB_Strict) {
    vector<string> nodes = { "S","X","Y" };
    std::unordered_map<string, std::size_t> idx;
    for (std::size_t i = 0; i < nodes.size(); ++i) idx[nodes[i]] = i;
    auto index_of = [&](const string& s) { return idx[s]; };

    vector<vector<pair<string, double>>> adj(nodes.size());
    adj[idx["S"]] = { {"X",1.0}, {"Y",2.0} };
    adj[idx["X"]] = { {"Y",1.0} };

    vector<double> db(nodes.size(), INF);
    db[idx["S"]] = 0.0;

    vector<string> S = { "S" };
    double B = 2.0;
    auto out = sssp::relax_k_steps(adj, db, std::move(S), B, /*K*/5, index_of);

    EXPECT_DOUBLE_EQ(db[idx["X"]], 1.0);
    EXPECT_DOUBLE_EQ(db[idx["Y"]], 2.0);  // == B → not in W
    expect_set_eq(out.W_union, std::set<string>({ "S","X" }));
}

TEST(RelaxString, KZero) {
    vector<string> nodes = { "A","B" };
    std::unordered_map<string, std::size_t> idx;
    for (std::size_t i = 0; i < nodes.size(); ++i) idx[nodes[i]] = i;
    auto index_of = [&](const string& s) { return idx[s]; };

    vector<vector<pair<string, double>>> adj(nodes.size());
    adj[idx["A"]] = { {"B",1.0} };

    vector<double> db(nodes.size(), INF);
    db[idx["A"]] = 0.0;

    vector<string> S = { "A" };
    auto out = sssp::relax_k_steps(adj, db, std::move(S), /*B*/INF, /*K*/0, index_of);

    expect_set_eq(out.W_union, std::set<string>({ "A" }));
    EXPECT_TRUE(std::isinf(db[idx["B"]]));
}
