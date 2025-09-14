// tests/test_d0.cpp
#include <gtest/gtest.h>
#include <list>
#include <vector>
#include <utility>
#include <limits>
#include <algorithm>

#include "ds_common.hpp"
#include "d0.hpp"

using KeyT = int;

static std::list<ds::Node<KeyT>> to_list(const std::vector<std::pair<KeyT, double>>& v) {
    std::list<ds::Node<KeyT>> out;
    for (auto& p : v) out.push_back(ds::Node<KeyT>{p.first, p.second});
    return out;
}

static std::size_t count_blocks(const D0<KeyT>& d0) {
    return static_cast<std::size_t>(std::distance(d0.begin(), d0.end()));
}

static double block_min(const ds::Block<KeyT>& b) {
    double mn = std::numeric_limits<double>::infinity();
    for (const auto& n : b.items) mn = std::min(mn, n.value);
    return mn;
}

TEST(D0, EmptyAtStart) {
    D0<KeyT> d0(3);
    EXPECT_TRUE(d0.empty());
    EXPECT_EQ(d0.size(), 0u);
    EXPECT_EQ(count_blocks(d0), 0u);

    double second = -1.0; // יישאר אינסוף כי remaining>0
    auto res = d0.pull(&second, d0.maxBlockSize());
    EXPECT_TRUE(res.first.empty());
    EXPECT_EQ(res.second, d0.maxBlockSize());
    EXPECT_TRUE(std::isinf(second));
}

TEST(D0, BatchPrepend_EmptyInput_NoChange) {
    D0<KeyT> d0(4);
    std::list<ds::Node<KeyT>> empty;
    auto it = d0.batchPrepend(std::move(empty));
    EXPECT_TRUE(d0.empty());
    EXPECT_EQ(it, d0.end());
    EXPECT_EQ(count_blocks(d0), 0u);
}

TEST(D0, BatchPrepend_ThenPullM_ComputesSecondValFromNextBlockMin) {
    // maxBlockSize=3, נכניס 7 פריטים => שני בלוקים מלאים ועוד בלוק של 1
    D0<KeyT> d0(3);
    auto items = to_list({
        {1, 1.2}, {2, 0.5}, {3, 0.7}, {4, 1.0}, {5, 2.3}, {6, 2.1}, {7, -0.4}
        });
    d0.batchPrepend(std::move(items));

    ASSERT_EQ(d0.size(), 7u);
    ASSERT_EQ(count_blocks(d0), 3u);

    // נמשוך maxBlockSize פריטים
    double second = std::numeric_limits<double>::quiet_NaN();
    auto r = d0.pull(&second, d0.maxBlockSize());
    EXPECT_EQ(r.second, 0u);
    EXPECT_EQ(r.first.size(), d0.maxBlockSize());
    EXPECT_EQ(d0.size(), 4u);

    // second אמור להיות ה-min של הבלוק הראשון הלא ריק שנותר
    ASSERT_FALSE(d0.empty());
    const auto& first_blk_left = *d0.begin();
    EXPECT_DOUBLE_EQ(second, block_min(first_blk_left));
}

TEST(D0, MultipleBatchPrepends_LIFOByBatch) {
    D0<KeyT> d0(2);

    // Batch A
    auto a = to_list({ {10, 1.0}, {11, 2.0}, {12, 3.0} });
    d0.batchPrepend(std::move(a));

    // Batch B (נוסף אחרי A, צריך לבוא לפניו ברשימת הבלוקים)
    auto b = to_list({ {20, -1.0}, {21, -2.0} });
    d0.batchPrepend(std::move(b));

    ASSERT_EQ(count_blocks(d0), 3u);

    // pull ראשון (maxBlockSize=2) צריך למשוך את שניהם של B
    double second = 0.0;
    auto r1 = d0.pull(&second, d0.maxBlockSize());
    ASSERT_EQ(r1.second, 0u);
    ASSERT_EQ(r1.first.size(), 2u);
    auto it = r1.first.begin();
    EXPECT_EQ(it->key, 20); ++it;
    EXPECT_EQ(it->key, 21);

    // pull שני אמור להתחיל למשוך מ-A
    auto r2 = d0.pull(&second, d0.maxBlockSize());
    ASSERT_EQ(r2.second, 0u);
    ASSERT_EQ(r2.first.size(), 2u);
    it = r2.first.begin();
    EXPECT_EQ(it->key, 10); ++it;
    EXPECT_EQ(it->key, 11);

    // pull שלישי – נשאר {12}
    auto r3 = d0.pull(&second, d0.maxBlockSize());
    EXPECT_EQ(r3.first.size(), 1u);
    EXPECT_EQ(r3.second, d0.maxBlockSize() - 1); // חסרים כדי להשלים maxBlockSize
    ASSERT_EQ(r3.first.begin()->key, 12);

    EXPECT_TRUE(d0.empty());
    EXPECT_EQ(count_blocks(d0), 0u);
}

TEST(D0, PullAcrossBlocks_SecondValReflectsNextBlock) {
    // maxBlockSize=4, 7 פריטים => בלוק של 4 ואז בלוק של 3
    D0<KeyT> d0(4);
    auto items = to_list({
        {1,  5.0}, {2,  1.0}, {3,  3.0}, {4,  7.0},
        {5,  2.5}, {6,  6.0}, {7, -4.0}
        });
    d0.batchPrepend(std::move(items));
    ASSERT_EQ(d0.size(), 7u);
    ASSERT_EQ(count_blocks(d0), 2u);

    double second = 0.0;
    auto r = d0.pull(&second, d0.maxBlockSize()); // מושך 4, נשארים 3
    EXPECT_EQ(r.second, 0u);
    EXPECT_EQ(r.first.size(), 4u);
    ASSERT_FALSE(d0.empty());
    EXPECT_EQ(second, block_min(*d0.begin())); // המינימום של הבלוק שנותר
}

TEST(D0, PullMoreThanAvailable_ReturnsRemainingAndDoesNotSetSecond) {
    D0<KeyT> d0(3);
    auto items = to_list({ {1,1.0}, {2,2.0} }); // רק שני פריטים
    d0.batchPrepend(std::move(items));

    const std::size_t want = 5;
    double second = 12345.0;
    auto r = d0.pull(&second, want);

    EXPECT_EQ(r.first.size(), 2u);             // כל מה שהיה
    EXPECT_EQ(r.second, want - 2u);            // כמה שחסר
    EXPECT_TRUE(d0.empty());
    // לפי המימוש: בתחילת pull מאפסים ל-infinity, וכש-remaining>0 לא מחשבים second
    EXPECT_TRUE(std::isinf(second));
}

TEST(D0, MEquals1_SingleItemBlocksAndFIFOWithinBlock) {
    D0<KeyT> d0(1);
    auto items = to_list({ {1, 3.0}, {2, 2.0}, {3, 1.0} });
    d0.batchPrepend(std::move(items));
    ASSERT_EQ(d0.size(), 3u);
    ASSERT_EQ(count_blocks(d0), 3u);

    for (int i = 0; i < 3; ++i) {
        double s = 0.0;
        auto p = d0.pull(&s, d0.maxBlockSize());
        EXPECT_EQ(p.second, 0u);
        EXPECT_EQ(p.first.size(), 1u);

        // סכום גדלי הבלוקים תואם ל-size_
        std::size_t sum = 0;
        for (auto it = d0.begin(); it != d0.end(); ++it) sum += it->items.size();
        EXPECT_EQ(sum, d0.size());
    }
    EXPECT_TRUE(d0.empty());
}

TEST(D0, ExactMultipleOfM_NoLeftoverBlocks) {
    D0<KeyT> d0(3);
    auto items = to_list({ {1,1},{2,2},{3,3},{4,4},{5,5},{6,6} }); // 6=2*3
    d0.batchPrepend(std::move(items));
    EXPECT_EQ(d0.size(), 6u);
    EXPECT_EQ(count_blocks(d0), 2u);

    double s = 0.0;
    auto p1 = d0.pull(&s, d0.maxBlockSize()); EXPECT_EQ(p1.second, 0u); EXPECT_EQ(p1.first.size(), 3u);
    auto p2 = d0.pull(&s, d0.maxBlockSize()); EXPECT_EQ(p2.second, 0u); EXPECT_EQ(p2.first.size(), 3u);
    EXPECT_TRUE(d0.empty());
    EXPECT_EQ(count_blocks(d0), 0u);
}

TEST(D0, SkipConsecutiveEmptyFrontBlocks) {
    D0<KeyT> d0(3);

    // נכין ידנית שני בלוקים ריקים מקדימה
    ds::Block<KeyT> b1; b1.items.clear();
    ds::Block<KeyT> b2; b2.items.clear();
    d0.blocks().push_back(std::move(b1));
    d0.blocks().push_back(std::move(b2));

    // ואז נוסיף batch אמיתי (נכנס לראש באמצעות splice)
    auto items = to_list({ {1, 1.0}, {2, 2.0}, {3, 3.0} });
    d0.batchPrepend(std::move(items));
    d0.size(); // לוודא שהקומפילר לא מתלונן על unused (לא חובה)
    EXPECT_EQ(d0.size(), 3u);

    double s = 0.0;
    auto p = d0.pull(&s, d0.maxBlockSize());
    EXPECT_EQ(p.second, 0u);
    EXPECT_EQ(p.first.size(), 3u);
    EXPECT_TRUE(d0.empty());
}
