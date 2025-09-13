// tests/test_d0.cpp
#include <gtest/gtest.h>
#include <list>
#include <vector>
#include <utility>
#include <limits>
#include <algorithm>

// עדכני נתיבים אם צריך:
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

static void expect_block_bounds_correct(const ds::Block<KeyT>& b) {
    if (b.items.empty()) {
        EXPECT_EQ(b.upper, ds::detail::neg_inf());
        return;
    }
    double mn = std::numeric_limits<double>::infinity();
    double mx = -std::numeric_limits<double>::infinity();
    for (const auto& kv : b.items) {
        mn = std::min(mn, kv.value);
        mx = std::max(mx, kv.value);
    }
    EXPECT_DOUBLE_EQ(b.upper, mx);
}

TEST(D0, EmptyAtStart) {
    D0<KeyT> d0(3);
    EXPECT_TRUE(d0.empty());
    EXPECT_EQ(d0.size(), 0u);
    EXPECT_EQ(count_blocks(d0), 0u);

    // pull כשהמבנה ריק
    std::pair<std::list<ds::Node<KeyT>>, std::size_t> res = d0.pull();
    EXPECT_TRUE(res.first.empty());
    EXPECT_EQ(res.second, 3u);
}

TEST(D0, BatchPrepend_EmptyInput_NoChange) {
    D0<KeyT> d0(4);
    std::list<ds::Node<KeyT>> empty;
    auto it = d0.batchPrepend(std::move(empty));
    EXPECT_TRUE(d0.empty());
    EXPECT_EQ(it, d0.end());
    EXPECT_EQ(count_blocks(d0), 0u);
}

TEST(D0, BatchPrepend_SplitsIntoBlocksAndBounds) {
    D0<KeyT> d0(3);

    // 7 פריטים => 3 בלוקים (3,3,1)
    auto items = to_list({
        {1, 1.2}, {2, 0.5}, {3, 0.7}, {4, 1.0}, {5, 2.3}, {6, 2.1}, {7, -0.4}
        });
    d0.batchPrepend(std::move(items));

    EXPECT_FALSE(d0.empty());
    EXPECT_EQ(d0.size(), 7u);
    EXPECT_EQ(count_blocks(d0), 3u);

    for (auto it = d0.begin(); it != d0.end(); ++it) {
        expect_block_bounds_correct(*it);
        for (const auto& kv : it->items) {
            EXPECT_LE(kv.value, it->upper + 1e-12);
        }
    }
}

TEST(D0, Pull_TakesMAndRemovesFrontBlocks) {
    D0<KeyT> d0(3);

    auto items = to_list({
        {1, 0.9}, {2, 1.1}, {3, 0.2}, {4, 0.4}, {5, 1.7}, {6, 1.6}, {7, 5.0}
        });
    d0.batchPrepend(std::move(items));

    ASSERT_EQ(d0.size(), 7u);
    ASSERT_EQ(count_blocks(d0), 3u);

    // pull ראשון
    auto r1 = d0.pull();
    EXPECT_EQ(r1.second, 0u);
    EXPECT_EQ(r1.first.size(), 3u);
    EXPECT_EQ(d0.size(), 4u);
    EXPECT_EQ(count_blocks(d0), 2u);

    // pull שני
    auto r2 = d0.pull();
    EXPECT_EQ(r2.second, 0u);
    EXPECT_EQ(r2.first.size(), 3u);
    EXPECT_EQ(d0.size(), 1u);
    EXPECT_EQ(count_blocks(d0), 1u);

    // pull שלישי – חסרים 2 כדי להשלים M=3
    auto r3 = d0.pull();
    EXPECT_EQ(r3.first.size(), 1u);
    EXPECT_EQ(r3.second, 2u);
    EXPECT_TRUE(d0.empty());
    EXPECT_EQ(count_blocks(d0), 0u);
}

TEST(D0, MultipleBatchPrepends_LIFOByBatch) {
    D0<KeyT> d0(2);

    // Batch A
    auto a = to_list({ {10, 1.0}, {11, 2.0}, {12, 3.0} });
    d0.batchPrepend(std::move(a));

    // Batch B (נכנס אחרי A, אמור לבוא לפניו)
    auto b = to_list({ {20, -1.0}, {21, -2.0} });
    d0.batchPrepend(std::move(b));

    ASSERT_EQ(count_blocks(d0), 3u);

    auto ra = d0.pull(); // אמור למשוך את שני של B
    ASSERT_EQ(ra.first.size(), 2u);
    {
        auto it = ra.first.begin();
        EXPECT_EQ(it->key, 20); ++it;
        EXPECT_EQ(it->key, 21);
    }

    auto rb = d0.pull(); // מתחיל למשוך מ-A
    ASSERT_EQ(rb.first.size(), 2u);
    {
        auto it = rb.first.begin();
        EXPECT_EQ(it->key, 10); ++it;
        EXPECT_EQ(it->key, 11);
    }

    auto rc = d0.pull(); // נשאר {12}
    EXPECT_EQ(rc.first.size(), 1u);
    EXPECT_EQ(rc.second, 1u);
    ASSERT_EQ(rc.first.begin()->key, 12);

    EXPECT_TRUE(d0.empty());
    EXPECT_EQ(count_blocks(d0), 0u);
}

TEST(D0, BoundsRecomputedAfterPartialPull) {
    D0<KeyT> d0(3);

    auto items = to_list({
        {1, 5.0}, {2, 1.0}, {3, 3.0}, {4, 7.0}, {5, 2.5}, {6, 6.0}
        });
    d0.batchPrepend(std::move(items));

    auto r = d0.pull(); // מושך 3, הבלוק הראשון נמחק, השני נשאר
    EXPECT_EQ(r.second, 0u);
    EXPECT_EQ(r.first.size(), 3u);

    ASSERT_FALSE(d0.empty());
    const auto& blk = *d0.begin();
    EXPECT_EQ(blk.items.size(), 3u);
    EXPECT_DOUBLE_EQ(blk.upper, 7.0);
}


TEST(D0, PartialPullAcrossBlocks_RecomputesSecondBlockBounds) {
    // נגדיר M=4. נכניס 7 פריטים => tmp ייצור בלוקים של 4 ואז 3,
    // ואז splice לראש (כך שהבלוק האחרון בתור יצירת tmp יהיה בחזית).
    D0<KeyT> d0(4);

    // 7 פריטים — הבלוק הקדמי (אחרי ה-batchPrepend) בגודל 3
    auto items = to_list({
        {1,  5.0}, {2,  1.0}, {3,  3.0}, {4,  7.0},
        {5,  2.5}, {6,  6.0}, {7, -4.0}
        });
    d0.batchPrepend(std::move(items));

    ASSERT_EQ(d0.size(), 7u);
    ASSERT_EQ(std::distance(d0.begin(), d0.end()), 2); // 2 בלוקים: קדמי 3, אחורי 4

    // pull אחד: צריך לקחת 4 פריטים — 3 מהבלוק הראשון ועוד 1 מהשני
    auto r = d0.pull();
    EXPECT_EQ(r.second, 0u);
    EXPECT_EQ(r.first.size(), 4u);

    // עכשיו הבלוק בחזית הוא זה שנפגע חלקית (נשארו בו 3 מתוך 4)
    ASSERT_FALSE(d0.empty());
    const auto& blk = *d0.begin();
    EXPECT_EQ(blk.items.size(), 3u);

    // חשבי ידנית את lower/upper של 3 הפריטים שנותרו בבלוק השני
    // (תלוי מהם ארבעת הפריטים שנמשכו; אנחנו יודעים שנמשכו ראשונים לפי סדר הכנסה)
    // לפני המשיכה, הבלוק האחורי הכיל {4:7.0, 5:2.5, 6:6.0, 7:-4.0}
    // משכנו פריט אחד ראשון ממנו (4:7.0), נשארו {5:2.5, 6:6.0, 7:-4.0}
    EXPECT_DOUBLE_EQ(blk.upper, 6.0);
}

TEST(D0, StableOrderWithinBlock_FIFO) {
    // נוודא שהסדר היחסי של פריטים בתוך בלוק נשמר במשיכה
    D0<KeyT> d0(5);

    // נכניס 5 פריטים בדיוק (בלוק יחיד)
    auto items = to_list({
        {10, 5.0}, {11, 4.0}, {12, 3.0}, {13, 2.0}, {14, 1.0}
        });
    d0.batchPrepend(std::move(items));

    // pull יחיד יחזיר את כל הבלוק; נבדוק שהסדר הוא בדיוק לפי ההכנסה
    auto r = d0.pull();
    ASSERT_EQ(r.second, 0u);
    ASSERT_EQ(r.first.size(), 5u);

    std::vector<int> keys;
    for (const auto& kv : r.first) keys.push_back(kv.key);

    std::vector<int> expected{ 10,11,12,13,14 };
    EXPECT_EQ(keys, expected);
}

TEST(D0, MEquals1_AllSingleItemBlocks) {
    D0<KeyT> d0(1);
    auto items = to_list({ {1, 3.0}, {2, 2.0}, {3, 1.0} });
    auto it_first = d0.batchPrepend(std::move(items));
    ASSERT_NE(it_first, d0.end());
    EXPECT_EQ(d0.size(), 3u);
    EXPECT_EQ(std::distance(d0.begin(), d0.end()), 3);

    // כל pull מוציא פריט אחד
    for (int i = 0; i < 3; ++i) {
        auto p = d0.pull();
        EXPECT_EQ(p.second, 0u);
        EXPECT_EQ(p.first.size(), 1u);
        // סכום גדלים = size_
        std::size_t sum = 0;
        for (auto it = d0.begin(); it != d0.end(); ++it) sum += it->items.size();
        EXPECT_EQ(sum, d0.size());
    }
    EXPECT_TRUE(d0.empty());
}

TEST(D0, ExactMultipleOfM_NoGhostBlocks) {
    D0<KeyT> d0(3);
    auto items = to_list({ {1,1},{2,2},{3,3},{4,4},{5,5},{6,6} }); // 6=2*3
    d0.batchPrepend(std::move(items));
    EXPECT_EQ(d0.size(), 6u);
    EXPECT_EQ(std::distance(d0.begin(), d0.end()), 2);

    // שני pull-ים מרוקנים הכול בלי להשאיר בלוק ריק
    auto p1 = d0.pull(); EXPECT_EQ(p1.second, 0u); EXPECT_EQ(p1.first.size(), 3u);
    auto p2 = d0.pull(); EXPECT_EQ(p2.second, 0u); EXPECT_EQ(p2.first.size(), 3u);
    EXPECT_TRUE(d0.empty());
    EXPECT_EQ(std::distance(d0.begin(), d0.end()), 0);
}

TEST(D0, SkipConsecutiveEmptyFrontBlocks) {
    D0<KeyT> d0(3);

    // נכין ידנית מצב עם שני בלוקים ריקים מקדימה
    ds::Block<KeyT> b1; b1.items.clear(); 
    ds::Block<KeyT> b2; b2.items.clear(); 
    d0.blocks().push_back(std::move(b1));
    d0.blocks().push_back(std::move(b2));

    // ואז נוסיף batch אמיתי (יגיע לראש)
    auto items = to_list({ {1, 1.0}, {2, 2.0}, {3, 3.0} });
    d0.batchPrepend(std::move(items));
    EXPECT_EQ(d0.size(), 3u);

    // pull צריך לדלג על הריקים ולמשוך מהבלוק עם הפריטים
    auto p = d0.pull();
    EXPECT_EQ(p.second, 0u);
    EXPECT_EQ(p.first.size(), 3u);
    EXPECT_TRUE(d0.empty());
}
