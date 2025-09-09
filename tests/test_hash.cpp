// tests/test_hash.cpp
#include <gtest/gtest.h>
#include <list>
#include <vector>
#include <utility>

#include "ds_common.hpp"
#include "hash.hpp"

using KeyT = int;

struct HandleFixture {
    std::list<ds::Block<KeyT>> blocks;
    std::list<ds::KV<KeyT>>* itemsPtr = nullptr;

    std::pair<std::list<ds::Block<KeyT>>::iterator,
        std::list<ds::KV<KeyT>>::iterator>
        make_block_with_items(const std::vector<std::pair<KeyT, double>>& kvs, std::size_t pick_index) {
        ds::Block<KeyT> b;
        for (auto& p : kvs) b.items.push_back(ds::KV<KeyT>{p.first, p.second});
        b.recompute_lower();
        b.recompute_upper();
        blocks.push_back(std::move(b));
        auto bit = std::prev(blocks.end());
        itemsPtr = &bit->items;
        auto it = bit->items.begin();
        for (std::size_t i = 0; i < pick_index && it != bit->items.end(); ++i) ++it;
        return { bit, it };
    }
};

TEST(Hash, EmptyAtStart) {
    Hash<KeyT> h;
    EXPECT_TRUE(h.empty());
    EXPECT_EQ(h.size(), 0u);
    EXPECT_FALSE(h.contains(123));
    EXPECT_EQ(h.get(123), nullptr);
}

TEST(Hash, InsertAndGet) {
    Hash<KeyT> h(8);

    HandleFixture fx;
    auto pair1 = fx.make_block_with_items({ {1,1.1},{2,2.2},{3,3.3} }, 1); // key=2
    auto bit = pair1.first;
    auto it = pair1.second;

    ds::Handle<KeyT> handle;
    handle.tier = ds::Tier::D0;
    handle.blockIt = bit;
    handle.itemIt = it;

    const KeyT key = it->key;
    const double val = it->value;
    ds::KV<KeyT>* addr_item = &(*it);

    bool inserted = h.insert(key, handle);
    EXPECT_TRUE(inserted);
    EXPECT_FALSE(h.empty());
    EXPECT_EQ(h.size(), 1u);
    EXPECT_TRUE(h.contains(key));

    auto* got = h.get(key);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->tier, ds::Tier::D0);
    EXPECT_EQ(got->key(), key);
    EXPECT_DOUBLE_EQ(got->value(), val);
    EXPECT_TRUE(&(*got->itemIt) == addr_item); // השוואת מצביעים בטוחה
}

TEST(Hash, InsertRvalueOverload) {
    Hash<KeyT> h;

    HandleFixture fx;
    auto pair1 = fx.make_block_with_items({ {10,5.5} }, 0);
    auto bit = pair1.first;
    auto it = pair1.second;

    ds::Handle<KeyT> handle;
    handle.tier = ds::Tier::D1;
    handle.blockIt = bit;
    handle.itemIt = it;

    KeyT k = 10;
    bool inserted = h.insert(std::move(k), std::move(handle));
    EXPECT_TRUE(inserted);
    EXPECT_TRUE(h.contains(10));
    EXPECT_NE(h.get(10), nullptr);
}

TEST(Hash, DuplicateInsertFailsSizeUnchanged) {
    Hash<KeyT> h;

    HandleFixture fx;
    auto pair1 = fx.make_block_with_items({ {7,0.7} }, 0);
    auto bit = pair1.first;
    auto it = pair1.second;

    ds::Handle<KeyT> a; a.tier = ds::Tier::D0; a.blockIt = bit; a.itemIt = it;
    ds::Handle<KeyT> b = a;

    EXPECT_TRUE(h.insert(7, a));
    EXPECT_FALSE(h.insert(7, b)); // כבר קיים
    EXPECT_EQ(h.size(), 1u);
}

TEST(Hash, UpsertOverwritesExistingHandle) {
    Hash<KeyT> h;

    HandleFixture fx;
    auto p1 = fx.make_block_with_items({ {1,1.0},{2,2.0} }, 0); // key=1
    auto bit1 = p1.first; auto it1 = p1.second;

    auto p2 = fx.make_block_with_items({ {1,9.9} }, 0);         // key=1 במקום אחר
    auto bit2 = p2.first; auto it2 = p2.second;

    ds::Handle<KeyT> h1; h1.tier = ds::Tier::D0; h1.blockIt = bit1; h1.itemIt = it1;
    ds::Handle<KeyT> h2; h2.tier = ds::Tier::D1; h2.blockIt = bit2; h2.itemIt = it2;

    h.insert(1, h1);
    ASSERT_NE(h.get(1), nullptr);
    EXPECT_EQ(h.get(1)->tier, ds::Tier::D0);
    EXPECT_TRUE(&(*h.get(1)->itemIt) == &(*it1));

    h.upsert(1, h2); // מחליף
    ASSERT_NE(h.get(1), nullptr);
    EXPECT_EQ(h.get(1)->tier, ds::Tier::D1);
    EXPECT_TRUE(&(*h.get(1)->itemIt) == &(*it2));
    EXPECT_EQ(h.size(), 1u);
}

TEST(Hash, EraseAndClear) {
    Hash<KeyT> h;

    HandleFixture fx;
    auto p = fx.make_block_with_items({ {3,3.3},{4,4.4} }, 0);
    auto bit = p.first;
    auto it0 = p.second;
    auto it1 = std::next(bit->items.begin());

    ds::Handle<KeyT> h3; h3.tier = ds::Tier::D0; h3.blockIt = bit; h3.itemIt = it0;
    ds::Handle<KeyT> h4; h4.tier = ds::Tier::D0; h4.blockIt = bit; h4.itemIt = it1;

    EXPECT_TRUE(h.insert(3, h3));
    EXPECT_TRUE(h.insert(4, h4));
    EXPECT_EQ(h.size(), 2u);

    EXPECT_TRUE(h.erase(3));
    EXPECT_FALSE(h.contains(3));
    EXPECT_EQ(h.size(), 1u);

    EXPECT_FALSE(h.erase(999));
    EXPECT_EQ(h.size(), 1u);

    h.clear();
    EXPECT_TRUE(h.empty());
    EXPECT_EQ(h.size(), 0u);
}

TEST(Hash, ConstGet) {
    Hash<KeyT> h;

    HandleFixture fx;
    auto p = fx.make_block_with_items({ {8,8.8} }, 0);
    auto bit = p.first;
    auto it = p.second;

    ds::Handle<KeyT> hh; hh.tier = ds::Tier::D1; hh.blockIt = bit; hh.itemIt = it;
    h.insert(8, hh);

    const Hash<KeyT>& ch = h;
    auto* got = ch.get(8);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->tier, ds::Tier::D1);
    EXPECT_EQ(got->key(), 8);
    EXPECT_DOUBLE_EQ(got->value(), 8.8);
}
