// tests/test_d1.cpp  (גרסת תאימות C++14)
#include <gtest/gtest.h>
#include <list>
#include <vector>
#include <limits>
#include <algorithm>
#include <random>
#include <set>
#include <cmath>

#include "ds_common.hpp"
#include "d1.hpp"

using Key = int;
using Node = ds::Node<Key>;

static std::list<Node> make_list(const std::vector<std::pair<Key, double>>& v) {
    std::list<Node> L;
    for (size_t i = 0; i < v.size(); ++i) {
        L.push_back(Node{ v[i].first, v[i].second });
    }
    return L;
}

/* ───────────────────────────── בסיס ───────────────────────────── */

TEST(D1, Construct_SentinelPresent_ChooseBindsToSentinel) {
    const std::size_t M = 8;
    const double B = 100.0;
    D1<Key> d1(M, B);
    D1<Key>::BlockIt it = d1.choose_block_for_value(B);
    (void)it;
    SUCCEED();
}

TEST(D1, InsertBlock_BeforeSentinel_ThenPullConsumesInListOrder) {
    D1<Key> d1(4, 100.0);

    D1<Key>::BlockIt where = d1.choose_block_for_value(100.0);

    (void)d1.insert_block(make_list({ {1,5.0},{2,7.0},{3,9.0} }), 30.0, where);
    (void)d1.insert_block(make_list({ {4,15.0},{5,20.0} }), 80.0, where);

    std::pair<std::list<Node>, double> res = d1.pull(4);
    std::list<Node>& out = res.first;
    double second = res.second;

    std::vector<double> got;
    for (std::list<Node>::const_iterator it = out.begin(); it != out.end(); ++it)
        got.push_back(it->value);

    ASSERT_EQ(got.size(), 4u);
    EXPECT_DOUBLE_EQ(got[0], 5.0);
    EXPECT_DOUBLE_EQ(got[1], 7.0);
    EXPECT_DOUBLE_EQ(got[2], 9.0);
    EXPECT_DOUBLE_EQ(got[3], 15.0);
    EXPECT_DOUBLE_EQ(second, 20.0);
}

TEST(D1, Pull_ExhaustsAllBlocks_SecondIsMaxOfOutput) {
    D1<Key> d1(4, 100.0);
    D1<Key>::BlockIt where = d1.choose_block_for_value(100.0);
    (void)d1.insert_block(make_list({ {1,2.0} }), 30.0, where);
    (void)d1.insert_block(make_list({ {2,4.0} }), 40.0, where);

    std::pair<std::list<Node>, double> res = d1.pull(10);
    const std::list<Node>& out = res.first;
    double second = res.second;

    ASSERT_EQ(out.size(), 2u);
    EXPECT_DOUBLE_EQ(second, 4.0);
}

TEST(D1, DeleteItem_RemovesBlockWhenItBecomesEmpty) {
    D1<Key> d1(8, 100.0);
    D1<Key>::BlockIt where = d1.choose_block_for_value(100.0);

    D1<Key>::BlockIt it = d1.insert_block(make_list({ {10,1.0} }), 10.0, where);
    ASSERT_FALSE(it->items.empty());

    d1.delete_item(it, it->items.begin());

    std::pair<std::list<Node>, double> res = d1.pull(1);
    const std::list<Node>& out = res.first;
    double second = res.second;

    EXPECT_TRUE(out.empty());
    EXPECT_TRUE(std::isinf(second));
}

TEST(D1, Split_HeavyDuplicates_RespectsPivotAndUppers) {
    D1<Key> d1(100, 100.0);
    D1<Key>::BlockIt where = d1.choose_block_for_value(100.0);

    D1<Key>::BlockIt it = d1.insert_block(
        make_list({
            std::make_pair(1,30.0), std::make_pair(2,30.0), std::make_pair(3,30.0),
            std::make_pair(4,30.0), std::make_pair(5,30.0),
            std::make_pair(6,10.0), std::make_pair(7,12.0), std::make_pair(8,15.0),
            std::make_pair(9,70.0), std::make_pair(10,80.0), std::make_pair(11,65.0)
            }),
        100.0, where
    );
    (void)d1.split(it);

    std::pair<std::list<Node>, double> res = d1.pull(1000);
    const std::list<Node>& out = res.first;
    double second = res.second;

    std::vector<double> vals;
    for (std::list<Node>::const_iterator it2 = out.begin(); it2 != out.end(); ++it2)
        vals.push_back(it2->value);

    std::multiset<double> expected;
    double arr[] = { 30,30,30,30,30, 10,12,15, 70,80,65 };
    for (size_t i = 0; i < sizeof(arr) / sizeof(arr[0]); ++i) expected.insert(arr[i]);
    std::multiset<double> got(vals.begin(), vals.end());
    EXPECT_EQ(got, expected);
    EXPECT_DOUBLE_EQ(second, 80.0);
}

TEST(D1, Fuzz_RandomSequences_DoNotCrashAndSecondMakesSense) {
    D1<Key> d1(8, 100.0);
    std::mt19937 rng(12345);
    std::uniform_real_distribution<double> distV(-50.0, 100.0);
    std::uniform_int_distribution<int> distOp(0, 9);

    D1<Key>::BlockIt where = d1.choose_block_for_value(100.0);
    D1<Key>::BlockIt last_it = where;

    for (int bi = 0; bi < 3; ++bi) {
        std::vector<std::pair<Key, double>> tmp;
        for (int j = 0; j < 6; ++j) tmp.push_back(std::make_pair(1000 + bi * 10 + j, distV(rng)));
        last_it = d1.insert_block(make_list(tmp), 100.0, where);
    }

    for (int step = 0; step < 60; ++step) {
        int op = distOp(rng);
        if (op <= 2) {
            std::vector<std::pair<Key, double>> tmp;
            for (int j = 0; j < 3; ++j) tmp.push_back(std::make_pair(2000 + step * 10 + j, distV(rng)));
            last_it = d1.insert_block(make_list(tmp), 100.0, where);
        }
        else if (op <= 4) {
            if (last_it != where && last_it->items.size() >= 2) (void)d1.split(last_it);
        }
        else if (op == 5) {
            (void)d1.pull(1);
        }
        else {
            std::size_t cnt = 1 + (step % 5);
            (void)d1.pull(cnt); // sanity בלבד
        }
    }
    SUCCEED();
}
