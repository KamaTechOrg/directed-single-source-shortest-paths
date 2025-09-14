#pragma once

#include <list>
#include <map>
#include <cstddef>
#include <utility>
#include <vector>
#include <algorithm>
#include <cassert>
#include "ds_common.hpp"
#include <optional>
#include <iterator>
#include <limits>

template <class Key>
class D1 {
public:
    using Node = ds::Node<Key>;
    using Block = ds::Block<Key>;
    using BlockList = std::list<Block>;
    using BlockIt = typename BlockList::iterator;
    using CBlockIt = typename BlockList::const_iterator;
    using ItemIt = typename std::list<Node>::iterator;

    explicit D1(std::size_t M, double B);

    /*std::size_t M()     const noexcept;
    bool empty() const noexcept;*/

    BlockIt choose_block_for_value(double value);

    BlockIt split(BlockIt blockIt);

    BlockIt insert_block(std::list<Node>&& items, double upper, BlockIt where);

    void delete_block(BlockIt it);

    void delete_item(BlockIt bIt, ItemIt iIt);

    void add_node_in_tree(double upper, BlockIt it);

    void delete_node_in_tree(double upper, BlockIt it);

    std::pair<std::list<Node>, double> 
    pull(std::size_t count);


private:
    using TauKey = std::pair<double, const void*>;

    // Tree index ordered by τ
    std::map<TauKey, BlockIt> tree_;

    // linked list of blocks
    BlockList blocks_;

    // block capacity and bias
    std::size_t M_{};
    double B_{};
};

// ======================== Implementations ========================

template<class Key>
D1<Key>::D1(std::size_t M, double B) : M_(M), B_(B) {
    assert(M_ > 0);
    typename D1<Key>::Block sentinel;  
    sentinel.upper = B_;               
    auto it = blocks_.insert(blocks_.end(), std::move(sentinel));
    typename D1<Key>::TauKey k{ it->upper, static_cast<const void*>(&(*it)) };
    tree_.emplace(k, it);
}


//template <class Key>
//std::size_t D1<Key>::M() const noexcept {
//    return M_;
//}
//
//template <class Key>
//bool D1<Key>::empty() const noexcept {
//    if (blocks_.empty()) return true;
//    for (const auto& b : blocks_) {
//        if (!b.items.empty()) return false;
//    }
//    return true;
//}

template <class Key>
typename D1<Key>::BlockIt
D1<Key>::choose_block_for_value(double value)
{
    assert(!tree_.empty() && "D1 tree must contain at least the sentinel block");
    assert(value <= B_ && "value exceeds global upper bound B");
    TauKey probe{ value, nullptr };
    auto it = tree_.lower_bound(probe);
    if (it != tree_.end()) {
        return it->second;
    }
    auto last = std::prev(tree_.end());
    return last->second;
}


template <class Key>
typename D1<Key>::BlockIt
D1<Key>::split(BlockIt blockIt)
{
    using ItemItT = typename D1<Key>::ItemIt;
    const std::size_t n = blockIt->items.size();
    const std::size_t k = n / 2;
    std::vector<ItemItT> idx;
    idx.reserve(n);
    for (auto i = blockIt->items.begin(); i != blockIt->items.end(); ++i)
        idx.push_back(i);
    auto mid = idx.begin() + static_cast<std::ptrdiff_t>(k);
    std::nth_element(idx.begin(), mid, idx.end(),
        [](const ItemItT& a, const ItemItT& b) { return a->value < b->value; });
    const double pivot = (*mid)->value;
    std::size_t cnt_lt = 0, cnt_eq = 0;
    for (const auto& itItem : idx) {
        if (itItem->value < pivot) ++cnt_lt;
        else if (itItem->value == pivot) ++cnt_eq;
    }
    const std::size_t need_left = k;
    const std::size_t eq_keep_left = (cnt_lt >= need_left) ? 0
        : std::min(cnt_eq, need_left - cnt_lt);
    std::size_t eq_kept_left = 0;
    Block newBlock;
    auto rightIt = blocks_.insert(std::next(blockIt), std::move(newBlock));
    for (auto cur = blockIt->items.begin(); cur != blockIt->items.end(); /* advance inside */) {
        const double v = cur->value;
        const bool move_right =
            (v > pivot) ||
            (v == pivot && eq_kept_left >= eq_keep_left);
        if (move_right) {
            auto to_move = cur++;
            rightIt->items.splice(rightIt->items.end(), blockIt->items, to_move);
        }
        else {
            if (v == pivot) ++eq_kept_left;
            ++cur;
        }
    }
    const double left_old_upper = blockIt->upper;

    delete_node_in_tree(left_old_upper, blockIt);
    add_node_in_tree(blockIt->upper, blockIt);
    add_node_in_tree(rightIt->upper, rightIt);
    return blockIt;
}

template <class Key>
typename D1<Key>::BlockIt
D1<Key>::insert_block(std::list<Node>&& items, double upper, BlockIt where) {
    Block b;
    b.items = std::move(items);
    b.upper = upper;
    auto it = blocks_.insert(where, std::move(b));
    add_node_in_tree(upper, it);
    return it;
}

template <class Key>
void D1<Key>::delete_block(BlockIt it) {
    if (it == blocks_.end()) return;
    delete_node_in_tree(it->upper, it);
    blocks_.erase(it);
}

template <class Key>
void D1<Key>::delete_item(BlockIt bIt, ItemIt iIt) {
    if (bIt == blocks_.end() || iIt == bIt->items.end()) return;
    bIt->items.erase(iIt);
    if (bIt->items.empty()) {
        delete_block(bIt);
        return;
    }
}


template <class Key>
void D1<Key>::add_node_in_tree(double upper, BlockIt it) {
    TauKey k{ upper, static_cast<const void*>(&(*it)) };
    tree_.emplace(k, it);
}


template <class Key>
void D1<Key>::delete_node_in_tree(double upper, BlockIt it) {
    TauKey k{ upper, static_cast<const void*>(&(*it)) };
    auto p = tree_.find(k);
    if (p != tree_.end()) tree_.erase(p);
}

template <class Key>
std::pair<std::list<typename D1<Key>::Node>, double>
D1<Key>::pull(std::size_t count)
{
    assert(count > 0 && "pull(count): count must be > 0");

    std::list<Node> out;
    double second_val = std::numeric_limits<double>::infinity();

    if (blocks_.empty()) {
        return { std::move(out), second_val };
    }

    auto bIt = blocks_.begin();
    std::size_t remaining = count;

    while (bIt != blocks_.end() && remaining > 0) {
        auto& items = bIt->items;

        while (!items.empty() && remaining > 0) {
            out.splice(out.end(), items, items.begin());
            --remaining;
        }

        if (items.empty()) {
            delete_node_in_tree(bIt->upper, bIt);
            bIt = blocks_.erase(bIt);
        }
        else {
            ++bIt;
        }
    }

    if (!blocks_.empty()) {
        for (const auto& blk : blocks_) {
            if (!blk.items.empty()) {
                const auto minIt = std::min_element(
                    blk.items.begin(), blk.items.end(),
                    [](const Node& a, const Node& b) { return a.value < b.value; }
                );
                second_val = minIt->value;  
                break;
            }
        }
    }
    else {
        if (!out.empty()) {
            auto it = std::max_element(out.begin(), out.end(),
                [](const Node& a, const Node& b) { return a.value < b.value; });
            second_val = it->value;
        }
    }

    return { std::move(out), second_val };
}
