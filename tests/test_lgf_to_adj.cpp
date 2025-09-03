
#include "lgf_to_adj.hpp"   
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <lemon/core.h>

#include <fstream>
#include <algorithm>
#include <vector>
#include <utility>
#include <chrono>
#include <cstdio>     

#ifndef RB2LEMON_TMP_DIR
#  define RB2LEMON_TMP_DIR "tools/rb2lemon/tmp_lgf"
#endif

static void ensure_dir_exists(const char* dir) {
#ifdef _WIN32
#include <direct.h>
    _mkdir(dir); 
#else
#include <sys/stat.h>
#include <sys/types.h>
    mkdir(dir, 0755);
#endif
}

static std::string write_temp_lgf(const std::string& content,
    const std::string& base = "lgf_test")
{
    ensure_dir_exists(RB2LEMON_TMP_DIR);

    using namespace std::chrono;
    auto stamp = duration_cast<microseconds>(
        high_resolution_clock::now().time_since_epoch()).count();

    std::string path = std::string(RB2LEMON_TMP_DIR)
        + "/" + base + "_" + std::to_string(stamp) + ".lgf";

    std::ofstream out(path);
    out << content;
    return path;
}

static void sort_adj(Adj& adj) {
    for (auto& nbrs : adj) {
        std::sort(nbrs.begin(), nbrs.end(),
            [](const auto& a, const auto& b) {
                if (a.first != b.first) return a.first < b.first;
                return a.second < b.second;
            });
    }
}

using ::testing::ElementsAre;
using Pair = std::pair<int, double>;


TEST(LgfDirected, BasicTriangle) {
    auto path = write_temp_lgf(
        "@nodes\n"
        "label id\n"
        "0     0\n"
        "1     1\n"
        "2     2\n"
        "@arcs\n"
        "    weight\n"
        "0   1   1.0\n"
        "1   2   2.0\n"
        "0   2   3.5\n",
        "basic_triangle"
    );

    Adj adj = lgf_to_adj(path, /*skipNeg=*/true);
    ASSERT_EQ(adj.size(), size_t{ 3 });

    sort_adj(adj);
    EXPECT_THAT(adj[0], ElementsAre(Pair{ 1,1.0 }, Pair{ 2,3.5 }));
    EXPECT_THAT(adj[1], ElementsAre(Pair{ 2,2.0 }));
    EXPECT_TRUE(adj[2].empty());

    std::remove(path.c_str());
}

TEST(LgfDirected, SelfLoopIgnored) {
    auto path = write_temp_lgf(
        "@nodes\n"
        "label id\n"
        "0     0\n"
        "@arcs\n"
        "    weight\n"
        "0   0   5\n",
        "self_loop"
    );

    Adj adj = lgf_to_adj(path, /*skipNeg=*/true);
    ASSERT_EQ(adj.size(), size_t{ 1 });
    EXPECT_TRUE(adj[0].empty());

    std::remove(path.c_str());
}

TEST(LgfDirected, NegativeWeightsToggle) {
    auto path = write_temp_lgf(
        "@nodes\n"
        "label id\n"
        "0     0\n"
        "1     1\n"
        "@arcs\n"
        "    weight\n"
        "0   1   -7.0\n",
        "neg_weight"
    );

    Adj adj_skip = lgf_to_adj(path, /*skipNeg=*/true);
    ASSERT_EQ(adj_skip.size(), size_t{ 2 });
    EXPECT_TRUE(adj_skip[0].empty());
    EXPECT_TRUE(adj_skip[1].empty());

    Adj adj_keep = lgf_to_adj(path, /*skipNeg=*/false);
    ASSERT_EQ(adj_keep.size(), size_t{ 2 });
    ASSERT_EQ(adj_keep[0].size(), size_t{ 1 });
    EXPECT_EQ(adj_keep[0][0].first, 1);
    EXPECT_DOUBLE_EQ(adj_keep[0][0].second, -7.0);

    std::remove(path.c_str());
}

TEST(LgfDirected, IsolatedNodes) {
    auto path = write_temp_lgf(
        "@nodes\n"
        "label id\n"
        "0     0\n"
        "1     1\n"
        "2     2\n"
        "@arcs\n"
        "    weight\n",  
        "isolated"
    );

    Adj adj = lgf_to_adj(path, /*skipNeg=*/true);
    ASSERT_EQ(adj.size(), size_t{ 3 });
    EXPECT_TRUE(adj[0].empty());
    EXPECT_TRUE(adj[1].empty());
    EXPECT_TRUE(adj[2].empty());

    std::remove(path.c_str());
}

TEST(LgfDirected, MissingFileThrows) {
    EXPECT_THROW(lgf_to_adj("tools/rb2lemon/tmp_lgf/no_such_file_XYZ.lgf", true),
        lemon::IoError);
}


TEST(LgfDirected, NonContiguousLabelsMapping) {
    auto path = write_temp_lgf(
        "@nodes\n"
        "label id\n"
        "10    100\n"
        "20    200\n"
        "5     500\n"
        "@arcs\n"
        "    weight\n"
        "5   10  1.0\n"   // 5->10  => id(500)->id(100)
        "20  5   2.0\n",  // 20->5  => id(200)->id(500)
        "non_contig"
    );

    Adj adj = lgf_to_adj(path, true);
    ASSERT_EQ(adj.size(), size_t{ 3 });

    auto sorted = adj;
    sort_adj(sorted);
    using P = std::pair<int, double>;

    EXPECT_TRUE(sorted[0].empty());                      // id=100 (label 10)
    EXPECT_THAT(sorted[1], ::testing::ElementsAre(P{ 2,2.0 })); // id=200 (label 20)
    EXPECT_THAT(sorted[2], ::testing::ElementsAre(P{ 0,1.0 })); // id=500 (label 5)

    std::remove(path.c_str());
}


TEST(LgfDirected, KeepsDuplicateArcs) {
    auto path = write_temp_lgf(
        "@nodes\n"
        "label id\n"
        "0 0\n"
        "1 1\n"
        "@arcs\n"
        "    weight\n"
        "0 1 1.0\n"
        "0 1 2.0\n",
        "dups"
    );

    Adj adj = lgf_to_adj(path, true);
    ASSERT_EQ(adj.size(), size_t{ 2 });
    ASSERT_EQ(adj[0].size(), size_t{ 2 });
    auto a0 = adj[0]; sort_adj(adj);
    EXPECT_EQ(adj[0][0].first, 1);
    EXPECT_EQ(adj[0][1].first, 1);
    EXPECT_DOUBLE_EQ(adj[0][0].second, 1.0);
    EXPECT_DOUBLE_EQ(adj[0][1].second, 2.0);

    std::remove(path.c_str());
}

TEST(LgfDirected, WeightParsingAndWhitespace) {
    auto path = write_temp_lgf(
        "@nodes\n"
        "label id\n"
        "0 0\n"
        "1 1\n"
        "2 2\n"
        "@arcs\n"
        "    weight\n"
        "0\t1\t0\n"        
        "0   2   0.5\n"    
        "1   2   1e-3\n",  
        "weights_ws"
    );

    Adj adj = lgf_to_adj(path, /*skipNeg=*/true);
    ASSERT_EQ(adj.size(), size_t{ 3 });

    sort_adj(adj);
    using P = std::pair<int, double>;
    EXPECT_THAT(adj[0], ::testing::ElementsAre(P{ 1,0.0 }, P{ 2,0.5 }));
    EXPECT_THAT(adj[1], ::testing::ElementsAre(P{ 2,1e-3 }));
    EXPECT_TRUE(adj[2].empty());

    std::remove(path.c_str());
}

TEST(LgfDirected, MissingWeightColumnThrows) {
    auto path = write_temp_lgf(
        "@nodes\n"
        "label id\n"
        "0 0\n"
        "1 1\n"
        "@arcs\n"   
        "0 1 2.0\n",
        "missing_weight"
    );

    EXPECT_THROW({ lgf_to_adj(path, true); }, lemon::FormatError);
    std::remove(path.c_str());
}
