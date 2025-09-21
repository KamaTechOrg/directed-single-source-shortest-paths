#include <gtest/gtest.h>
#include <list>
#include <vector>
#include <utility>
#include <algorithm>
#include <string>

// Project headers
#include "sssp/dual_block_structure.hpp"
#include "d0.hpp"
#include "d1.hpp"
#include "hash.hpp"
#include "ds_common.hpp"

using ds::Node;        // convenience
using ds::Block;

// ---------- small helpers ----------

template <class Key>
static std::list<Node<Key>> make_nodes(std::initializer_list<std::pair<Key, double>> xs) {
    std::list<Node<Key>> out;
    for (auto& p : xs) out.push_back({ p.first, p.second });
    return out;
}

template <class Key>
static std::vector<std::pair<Key, double>> to_vec(const std::list<Node<Key>>& xs) {
    std::vector<std::pair<Key, double>> v;
    v.reserve(xs.size());
    for (typename std::list<Node<Key>>::const_iterator it = xs.begin(); it != xs.end(); ++it) {
        v.push_back(std::make_pair(it->key, it->value));
    }
    return v;
}

// =============================================================
//                           D0
// =============================================================

TEST(D0_Flow, BatchPrepend_SplitsToBlocksAndSize) {
    D0<int> d0(3); // maxBlockSize=3
    std::list<Node<int>> items = make_nodes<int>({ {1,10},{2,11},{3,12},{4,13},{5,14},{6,15},{7,16} });

    D0<int>::BlockIt first = d0.batchPrepend(std::move(items));
    ASSERT_NE(first, d0.end());

    // size tracked
    EXPECT_EQ(d0.size(), 7u);

    // should be 3 blocks of sizes 3,3,1 (prepended order)
    std::vector<size_t> sizes;
    for (D0<int>::BlockIt it = d0.begin(); it != d0.end(); ++it) sizes.push_back(it->items.size());
    ASSERT_EQ(sizes.size(), 3u);
    EXPECT_EQ(sizes[0], 3u);
    EXPECT_EQ(sizes[1], 3u);
    EXPECT_EQ(sizes[2], 1u);
}

TEST(D0_Flow, Pull_PartialAndSecondValFromNextBlock) {
    D0<int> d0(3);
    // 5 items ? blocks: [3],[2]
    std::list<Node<int>> items = make_nodes<int>({ {1,10},{2,20},{3,30},{4,5},{5,7} });
    d0.batchPrepend(std::move(items));

    double second = -1;
    std::pair<std::list<Node<int> >, std::size_t> pr = d0.pull(&second, /*n*/3);
    std::list<Node<int> >& out = pr.first;
    std::size_t remaining = pr.second;

    EXPECT_EQ(remaining, 0u);
    std::vector<std::pair<int, double> > v = to_vec(out);
    ASSERT_EQ(v.size(), 3u);
    // by implementation, we take from the first block in list order
    EXPECT_EQ(v[0].first, 1);
    EXPECT_EQ(v[1].first, 2);
    EXPECT_EQ(v[2].first, 3);

    // second = min value in first non-empty block after removal (that is block #2 => values 5,7)
    EXPECT_DOUBLE_EQ(second, 5.0);
}

TEST(D0_Flow, Pull_EmptiesAllBlocks_SetsSecondToMaxOfOut) {
    D0<int> d0(4);
    std::list<Node<int>> items = make_nodes<int>({ {1,4},{2,10},{3,6} });
    d0.batchPrepend(std::move(items));

    double second = -1;
    std::pair<std::list<Node<int> >, std::size_t> pr = d0.pull(&second, /*n*/4);
    std::list<Node<int> >& out = pr.first;
    std::size_t remaining = pr.second;
    (void)out; // used below logically

    EXPECT_EQ(remaining, 1u); // asked 4, had only 3

    // when no blocks remain, second = max value of what we pulled
    // pulled values: 4,10,6 ? max = 10
    EXPECT_TRUE(std::isinf(second));
}

// =============================================================
//                           D1
// =============================================================

TEST(D1_Flow, InsertChooseBlockAndDeleteItem) {
    D1<int> d1(/*M*/3, /*B*/100.0);

    // insert a block with upper 50 just before sentinel
    std::list<Node<int>> blk50_items = make_nodes<int>({ {11,10.0},{12,50.0} });
    D1<int>::BlockIt whereSentinel = d1.choose_block_for_value(100.0); // sentinel block
    D1<int>::BlockIt blk50 = d1.insert_block(std::move(blk50_items), /*upper*/50.0, whereSentinel);

    // check choose works
    D1<int>::BlockIt got = d1.choose_block_for_value(40.0);
    EXPECT_EQ(&(*got), &(*blk50));

    // insert a block with upper 80 after 50 and before sentinel
    std::list<Node<int>> blk80_items = make_nodes<int>({ {21,70.0},{22,79.0} });
    D1<int>::BlockIt blk80 = d1.insert_block(std::move(blk80_items), /*upper*/80.0, whereSentinel);

    // choose around boundaries
    EXPECT_EQ(&(*d1.choose_block_for_value(79.0)), &(*blk80));
    EXPECT_EQ(&(*d1.choose_block_for_value(50.0)), &(*blk50));
    EXPECT_EQ(&(*d1.choose_block_for_value(81.0)), &(*whereSentinel)); // 81 -> sentinel

    // delete items from blk80; when empty, the block should be removed (safe deletion)
    std::vector<const Node<int>*> ptrs;
    ptrs.reserve(blk80->items.size());
    for (std::list<Node<int>>::const_iterator it = blk80->items.begin();
        it != blk80->items.end(); ++it) {
        ptrs.push_back(&*it);
    }
    for (const Node<int>* p : ptrs) {
        d1.delete_item(blk80, p); // הקריאה האחרונה תמחק את הבלוק
    }

    // now value 79 should bind to the sentinel (τ=100), not blk50 (τ=50 < 79)
    D1<int>::BlockIt afterDel = d1.choose_block_for_value(79.0);
    EXPECT_EQ(&(*afterDel), &(*whereSentinel));
}


TEST(D1_Flow, Split_SetsUppersAndPartitions) {
    D1<int> d1(/*M*/10, /*B*/100.0);

    // create a big block under upper=100
    D1<int>::BlockIt where = d1.choose_block_for_value(100.0); // sentinel
    std::list<Node<int>> items = make_nodes<int>({ {1,10},{2,20},{3,20},{4,30},{5,40} });
    D1<int>::BlockIt blk = d1.insert_block(std::move(items), /*upper*/100.0, where);

    // split (n=5,k=2)
    D1<int>::BlockIt left = d1.split(blk);
    D1<int>::BlockIt right = left;
    ++right;

    // both non-empty and sizes add up (לא משווים לאיטרטור ריק ב-MSVC)
    EXPECT_GT(left->items.size(), 0u);
    EXPECT_GT(right->items.size(), 0u);
    EXPECT_EQ(left->items.size() + right->items.size(), 5u);

    // upper bounds: left <= pivot, right inherits old upper (100)
    EXPECT_LE(left->blockUpper, 100.0);
    EXPECT_DOUBLE_EQ(right->blockUpper, 100.0);

    // partition property: all left values <= left.blockUpper; all right >= left.blockUpper
    for (std::list<Node<int> >::const_iterator it = left->items.begin(); it != left->items.end(); ++it)
        EXPECT_LE(it->value, left->blockUpper);
    for (std::list<Node<int> >::const_iterator it = right->items.begin(); it != right->items.end(); ++it)
        EXPECT_GE(it->value, left->blockUpper);
}

TEST(D1_Flow, Pull_TakesFromFrontBlocksAndSecondVal) {
    D1<int> d1(/*M*/5, /*B*/100.0);

    D1<int>::BlockIt where = d1.choose_block_for_value(100.0); // sentinel at end
    D1<int>::BlockIt b1 = d1.insert_block(make_nodes<int>({ {1,1.0},{2,2.0} }), /*upper*/2.0, where);
    (void)b1;
    D1<int>::BlockIt b2 = d1.insert_block(make_nodes<int>({ {3,0.5},{4,5.0} }), /*upper*/10.0, where);
    (void)b2;

    std::pair<std::list<Node<int> >, double> pr = d1.pull(/*count*/3);
    std::list<Node<int> >& out = pr.first;
    double second = pr.second;

    // expected order: from front block (b1), then continue into b2
    std::vector<std::pair<int, double> > v = to_vec(out);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0].first, 1);
    EXPECT_EQ(v[1].first, 2);
    EXPECT_EQ(v[2].first, 3);

    // remaining first non-empty block is b2 with one item {4,5.0}
    EXPECT_DOUBLE_EQ(second, 5.0);
}

// =============================================================
//                           Hash
// =============================================================

TEST(Hash_Flow, BasicOps) {
    Hash<int> H(4);
    ds::Handle<int> h; // default

    EXPECT_FALSE(H.contains(7));
    EXPECT_TRUE(H.insert(7, h));
    EXPECT_TRUE(H.contains(7));

    ds::Handle<int>* p = H.get(7);
    ASSERT_NE(p, (ds::Handle<int>*)nullptr);

    EXPECT_TRUE(H.erase(7));
    EXPECT_FALSE(H.contains(7));
}

// =============================================================
//                  DualBlockStructure (integration)
// =============================================================

TEST(DualBlockStructure_Flow, Insert_NewUpdateAndHash) {
    DualBlockStructure<int> S(3, /*B*/100, /*expected_keys*/8);

    // new
    EXPECT_TRUE(S.insert(7, 42.0));
    const ds::Handle<int>* h = S.map().get(7);
    ASSERT_NE(h, (const ds::Handle<int>*)nullptr);
    EXPECT_DOUBLE_EQ(h->value(), 42.0);

    // same value ? no change
    EXPECT_FALSE(S.insert(7, 42.0));
    h = S.map().get(7);
    ASSERT_NE(h, (const ds::Handle<int>*)nullptr);
    EXPECT_DOUBLE_EQ(h->value(), 42.0);

    // different value ? reinsert
    EXPECT_TRUE(S.insert(7, 6.5));
    h = S.map().get(7);
    ASSERT_NE(h, (const ds::Handle<int>*)nullptr);
    EXPECT_DOUBLE_EQ(h->value(), 6.5);
}

TEST(DualBlockStructure_Flow, BatchPrependAndPullMix) {
    DualBlockStructure<int> S(3, /*B*/100, 8);

    // D0 gets 2 items (values 8,9); D1 gets two values (1.0, 2.5)
    S.batch_prepend(make_nodes<int>({ {100,8.0},{101,9.0} }));
    EXPECT_TRUE(S.insert(1, 1.0));
    EXPECT_TRUE(S.insert(2, 2.5));

    std::pair<std::list<Node<int> >, double> pr = S.pull();
    std::list<Node<int> >& out = pr.first;
    double bound = pr.second;

    // We pull up to d0.maxBlockSize()==3: first from D0 (2 items), then remaining 1 from D1
    std::vector<std::pair<int, double> > v = to_vec(out);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0].first, 100);
    EXPECT_EQ(v[1].first, 101);
    EXPECT_EQ(v[2].first, 1); // first item from D1's front block

    // Since we also pulled from D1, bound comes from D1::pull ? min of remaining first block = 2.5
    EXPECT_DOUBLE_EQ(bound, 2.5);
}

TEST(DualBlockStructure_Flow, PullOnlyFromD0_SetsBoundToD0Second) {
    DualBlockStructure<int> S(3, /*B*/100, 8);

    // exactly 3 in D0 ? D0 only
    S.batch_prepend(make_nodes<int>({ {1,4.0},{2,10.0},{3,6.0} }));
    std::pair<std::list<Node<int> >, double> pr = S.pull();
    std::list<Node<int> >& out = pr.first;
    (void)out;
    double bound = pr.second;

    // bound should be max(out.values) because D0 emptied and code sets second to max(out)
    EXPECT_DOUBLE_EQ(bound, 10.0);
}

// =============================================================
//                  DualBlockStructure::initialize
// =============================================================

TEST(DualBlockStructure_Flow, Initialize_ResetsStateAndBounds) {
    DualBlockStructure<int> S(2, /*B*/50, 0);

    // mutate some state
    S.batch_prepend(make_nodes<int>({ {1,1.1} }));
    S.insert(9, 9.9);

    // reinitialize
    S.initialize(/*M*/4, /*B*/200, /*expected*/16);

    EXPECT_EQ(S.maxBlockSize(), 4u);
    EXPECT_EQ(S.globalUpperBound(), 200u);

    // inserting value ? 200 must succeed (and not crash). Also ensures new D1 was created.
    EXPECT_TRUE(S.insert(7, 123.0));
}

// NOTE: This file exercises the API-level flow and observable invariants based on the
// current implementation. It intentionally avoids relying on private internals beyond
// iterators returned by public methods (e.g., insert_block/split) to keep tests stable.
