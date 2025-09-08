//// tests/test_d0.cpp
//#include <gtest/gtest.h>
//#include <list>
//#include <vector>
//#include <utility>
//#include <algorithm>
//#include "ds_common.hpp"
//#include "d0.hpp"
//
//using KeyT = int;
//using D0T = D0<KeyT>;
//using KV = ds::KV<KeyT>;
//using Block = ds::Block<KeyT>;
//
//static std::list<KV> to_list(const std::vector<std::pair<KeyT, double>>& v) {
//    std::list<KV> out;
//    for (auto& [k, val] : v) out.push_back(KV{ k, val });
//    return out;
//}
//
//TEST(D0, EmptyAtStart) {
//    D0T d0(3);
//    EXPECT_TRUE(d0.empty());
//    EXPECT_EQ(d0.size(), 0u);
//    EXPECT_EQ(d0.blocks().size(), 0u);
//}
//
//TEST(D0, BatchPrependSingleBlock) {
//    D0T d0(4); // M=4
//    auto items = to_list({ {1,1.0},{2,2.0},{3,3.0} });
//    auto itFirst = d0.batchPrepend(std::move(items));
//    EXPECT_EQ(d0.size(), 3u);
//    ASSERT_NE(itFirst, d0.blocks().end());
//    EXPECT_EQ(itFirst->items.size(), 3u);
//    // סדר נשמר
//    auto it = itFirst->items.begin();
//    EXPECT_EQ(it->key, 1); ++it;
//    EXPECT_EQ(it->key, 2); ++it;
//    EXPECT_EQ(it->key, 3);
//    // גבולות
//    EXPECT_DOUBLE_EQ(itFirst->lower, 1.0);
//    EXPECT_DOUBLE_EQ(itFirst->upper, 3.0);
//}
//
//TEST(D0, BatchPrependMultipleBlocksAndOrder) {
//    D0T d0(3); // M=3 => ייווצרו 2 בלוקים
//    auto items = to_list({ {1,1.0},{2,2.0},{3,3.0},{4,4.0},{5,5.0} });
//    d0.batchPrepend(std::move(items));
//    // שני בלוקים: קדמי (1,2,3) ואחריו (4,5)
//    ASSERT_EQ(d0.blocks().size(), 2u);
//    auto itB = d0.blocks().begin();
//    ASSERT_EQ(itB->items.size(), 3u);
//    auto itI = itB->items.begin();
//    EXPECT_EQ((itI++)->key, 1);
//    EXPECT_EQ((itI++)->key, 2);
//    EXPECT_EQ((itI++)->key, 3);
//    ++itB;
//    ASSERT_EQ(itB->items.size(), 2u);
//    itI = itB->items.begin();
//    EXPECT_EQ((itI++)->key, 4);
//    EXPECT_EQ((itI++)->key, 5);
//    EXPECT_EQ(d0.size(), 5u);
//}
//
//TEST(D0, PullUpToM) {
//    D0T d0(3);
//    d0.batchPrepend(to_list({ {1,1.0},{2,2.0},{3,3.0},{4,4.0} }));
//    auto [out, deficit] = d0.pull(); // עד M=3
//    // יצאו 3, נשאר חסר 0 להשלים ל-3
//    EXPECT_EQ(deficit, 0u);
//    ASSERT_EQ(std::distance(out.begin(), out.end()), 3);
//    // ב-D0 נשאר 1 פריט (4)
//    EXPECT_EQ(d0.size(), 1u);
//    ASSERT_EQ(d0.blocks().size(), 1u);
//    // סדר יציאה נכון: 1,2,3
//    auto it = out.begin();
//    EXPECT_EQ((it++)->key, 1);
//    EXPECT_EQ((it++)->key, 2);
//    EXPECT_EQ((it++)->key, 3);
//}
//
//TEST(D0, PullWhenLessThanMAvailable) {
//    D0T d0(5); // M=5
//    d0.batchPrepend(to_list({ {10,1.0},{11,2.0} }));
//    auto [out, deficit] = d0.pull(); // ביקשנו 5, יש רק 2
//    EXPECT_EQ(std::distance(out.begin(), out.end()), 2);
//    EXPECT_EQ(deficit, 3u); // חסר 3 כדי להגיע ל-5
//    EXPECT_TRUE(d0.empty());
//    EXPECT_EQ(d0.blocks().size(), 0u);
//}
//
//TEST(D0, PullAcrossBlocksMaintainsFrontToBackOrder) {
//    D0T d0(2); // פיצול לבלוקים בגודל 2
//    d0.batchPrepend(to_list({ {1,1.0},{2,2.0},{3,3.0},{4,4.0},{5,5.0} })); // בלוקים: [1,2], [3,4], [5]
//    auto [out, deficit] = d0.pull(); // M=2 => מוציא 1,2
//    EXPECT_EQ(deficit, 0u);
//    EXPECT_EQ(std::distance(out.begin(), out.end()), 2);
//    auto it = out.begin();
//    EXPECT_EQ((it++)->key, 1);
//    EXPECT_EQ((it++)->key, 2);
//
//    // משיכה נוספת
//    auto [out2, deficit2] = d0.pull(); // יוציא 3,4
//    EXPECT_EQ(deficit2, 0u);
//    it = out2.begin();
//    EXPECT_EQ((it++)->key, 3);
//    EXPECT_EQ((it++)->key, 4);
//
//    // משיכה שלישית: ייקח את 5 (חסר 1 כדי להשלים ל-2)
//    auto [out3, deficit3] = d0.pull();
//    EXPECT_EQ(std::distance(out3.begin(), out3.end()), 1);
//    EXPECT_EQ(deficit3, 1u);
//    EXPECT_TRUE(d0.empty());
//}
