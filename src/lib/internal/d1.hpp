// D1: linked list of value-bounded blocks + τ-index (std::map).
// Provides O(log n) block selection by value and O(n) worst-case block split
// via BFPRT median-of-medians. Keeps tree_ (τ → BlockIt) consistent with blocks_.
// Invariant: tree_ is non-empty after construction (sentinel exists).
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
#include <functional>

template <class Key>
class D1 {
public:
    using Node = ds::Node<Key>;
    using Block = ds::Block<Key>;
    using BlockList = std::list<Block>;
    using BlockIt = typename BlockList::iterator;
    using CBlockIt = typename BlockList::const_iterator;
    using ItemIt = typename std::list<Node>::iterator;

    explicit D1(std::size_t maxBlockSize, double globalUpperBound);

    std::size_t maxBlockSize()     const noexcept;
    bool empty() const noexcept;

    BlockIt choose_block_for_value(double value);

    BlockIt split(BlockIt blockIt);

    BlockIt insert_block(std::list<Node>&& items, double blockUpper, BlockIt where);

    void delete_block(BlockIt it);

    // Safe deletion API: delete by a stable pointer (survives split/splice/pull).
    bool delete_item(BlockIt bIt, const Node * ptr);

    void add_node_in_tree(double blockUpper, BlockIt it);

    void delete_node_in_tree(double blockUpper, BlockIt it);

    std::pair<std::list<Node>, double> 
    pull(std::size_t count);


private:
    // τ-key ordering: (upper bound τ, block address). Address ensures uniqueness when τ ties.
    using TauKey = std::pair<double, const void*>;

    //struct TauLess {
    //    bool operator()(const TauKey & a, const TauKey & b) const noexcept {
    //        if (a.first < b.first) return true;
    //        if (b.first < a.first) return false;
    //            // Use std::less for pointers to get a strict total order across unrelated objects
    //            return std::less<const void*>()(a.second, b.second);
    //    }
    //};
    // Tree index ordered by τ
    std::map<TauKey, BlockIt> tree_;

    // linked list of blocks
    BlockList blocks_;

    // block capacity and bias
    std::size_t maxBlockSize_{};
    double globalUpperBound_{};
private:
    static ItemIt bfprt_select_(std::vector<ItemIt>& a, std::size_t k);
    static ItemIt bfprt_select_(std::vector<ItemIt>& a, std::size_t l, std::size_t r, std::size_t k);


    bool is_sentinel(BlockIt it) const {
        return it != blocks_.end() && std::next(it) == blocks_.end();
    }

    // Locate iterator in a block by a stable Node* (no UB on singular iterators).
    ItemIt find_in_block(BlockIt bIt, const Node * p) {
        if (bIt == blocks_.end() || p == nullptr) return (bIt == blocks_.end()
             ? ItemIt{} : bIt->items.end());
        for (auto it = bIt->items.begin(); it != bIt->items.end(); ++it)
            if (&*it == p) return it;
        return bIt->items.end();
    }


};

// ======================== Implementations ========================
// Construct D1 with block capacity M and a sentinel block whose τ=globalUpperBound.
// Initializes the block list with a single sentinel block and indexes it in the tree.
// Invariant: tree_ is non-empty after construction (always contains the sentinel).
template<class Key>
D1<Key>::D1(std::size_t maxBlockSize, double globalUpperBound) : maxBlockSize_(maxBlockSize), globalUpperBound_(globalUpperBound) {
    assert(maxBlockSize_ > 0);
    typename D1<Key>::Block sentinel;  
    sentinel.blockUpper = globalUpperBound_;
    auto it = blocks_.insert(blocks_.end(), std::move(sentinel));
    typename D1<Key>::TauKey k{ it->blockUpper, static_cast<const void*>(&(*it)) };
    tree_.emplace(k, it);
}

// Return the configured per-block capacity (M).
template <class Key>
std::size_t D1<Key>::maxBlockSize() const noexcept {
    return maxBlockSize_;
}
// True iff all blocks are empty (ignoring the presence of the sentinel itself).
// Performs a linear scan to detect any non-empty block.
template <class Key>
bool D1<Key>::empty() const noexcept {
    if (blocks_.empty()) return true;
    for (const auto& b : blocks_) {
        if (!b.items.empty()) return false;
    }
    return true;
}
// Return iterator to the first block whose upper bound τ ≥ value.
// Uses the tree_ (ordered by <τ, address>) via lower_bound for O(log n) lookup.
// Precondition: value ≤ globalUpperBound_.
template <class Key>
typename D1<Key>::BlockIt
D1<Key>::choose_block_for_value(double value)
{
    assert(!tree_.empty() && "D1 tree must contain at least the sentinel block");
    assert(value <= globalUpperBound_ && "value exceeds global upper bound B");
    TauKey probe{ value, nullptr };
    auto it = tree_.lower_bound(probe);
    if (it != tree_.end()) {
        return it->second;
    }
    auto last = std::prev(tree_.end());
    return last->second;
}

// Split the given block around its median value using BFPRT (median-of-medians):
// 1) Select median pivot in linear time.
// 2) Partition items: ≤ pivot stay left (tie-handling keeps exactly ⌊n/2⌋ on the left),
//    > pivot move to a newly inserted right-adjacent block.
// 3) Update τ: left τ := pivot, right τ := old left τ; update the tree_ entries.
//
// Returns iterator to the (updated) left block.
template <class Key>
typename D1<Key>::BlockIt
D1<Key>::split(BlockIt blockIt)
{
    using ItemItT = typename D1<Key>::ItemIt;

    const std::size_t n = blockIt->items.size();
    if (n < 2) return blockIt;
    const std::size_t k = n / 2; 
    std::vector<ItemItT> a;
    a.reserve(n);

   
    for (auto it = blockIt->items.begin(); it != blockIt->items.end(); ++it)
        a.push_back(it);
    assert(!a.empty());
    assert(k < a.size());

    ItemItT itK = bfprt_select_(a, k);
    const double pivot = itK->value;
    std::size_t cnt_lt = 0, cnt_eq = 0;
    for (const auto& it : a) {
        if (it->value < pivot) ++cnt_lt;
        else if (it->value == pivot) ++cnt_eq;
    }
    const std::size_t need_left = k;
    const std::size_t eq_keep_left = (cnt_lt >= need_left) ? 0
        : std::min(cnt_eq, need_left - cnt_lt);
    Block newBlock;
    auto rightIt = blocks_.insert(std::next(blockIt), std::move(newBlock));
    std::size_t eq_kept_left = 0;
    for (auto cur = blockIt->items.begin(); cur != blockIt->items.end(); ) {
        const double v = cur->value;
        const bool move_right =
            (v > pivot) || (v == pivot && eq_kept_left >= eq_keep_left);
        if (move_right) {
            auto to_move = cur++;
            rightIt->items.splice(rightIt->items.end(), blockIt->items, to_move);
        }
        else {
            if (v == pivot) ++eq_kept_left;
            ++cur;
        }
    }
    if (rightIt->items.empty()) {
        blocks_.erase(rightIt);
        return blockIt;
    }
    const double left_old_upper = blockIt->blockUpper;
    const double left_new_upper = pivot;
    const double right_new_upper = left_old_upper;

    delete_node_in_tree(left_old_upper, blockIt);
    blockIt->blockUpper = left_new_upper;
    add_node_in_tree(blockIt->blockUpper, blockIt);
    rightIt->blockUpper = right_new_upper;
    add_node_in_tree(rightIt->blockUpper, rightIt);
    return blockIt; 
}


// Create a new block with the given items and τ=blockUpper, insert it before 'where',
// and index it in tree_. Returns iterator to the inserted block.
template <class Key>
typename D1<Key>::BlockIt
D1<Key>::insert_block(std::list<Node>&& items, double blockUpper, BlockIt where) {
    Block b;
    b.items = std::move(items);
    b.blockUpper = blockUpper;
    auto it = blocks_.insert(where, std::move(b));
    add_node_in_tree(blockUpper, it);
    return it;
}
// Remove the block from both the tree_ and the block list (no-op for end()).
// Caller must ensure the iterator is valid and not already erased.
template <class Key>
void D1<Key>::delete_block(BlockIt it) {
    if (it == blocks_.end() || is_sentinel(it)) return;
    delete_node_in_tree(it->blockUpper, it);
    blocks_.erase(it);
}

// Erase a specific node by stable pointer; if the block becomes empty, delete it.
// Returns true if a node was removed, false otherwise. Never dereferences singular iterators.
template <class Key>
bool D1<Key>::delete_item(BlockIt bIt, const Node * ptr) {
    if (bIt == blocks_.end() || ptr == nullptr) return false;
    auto iIt = find_in_block(bIt, ptr);
    if (iIt == bIt->items.end()) return false; // not found / not in this block
    bIt->items.erase(iIt);
    if (bIt->items.empty() && !is_sentinel(bIt)) delete_block(bIt);
    return true;
}

// Index a block in the tree_ under key <blockUpper, address(block)>.
// The address disambiguates identical τ values to keep keys unique and ordered.
template <class Key>
void D1<Key>::add_node_in_tree(double blockUpper, BlockIt it) {
    TauKey k{ blockUpper, static_cast<const void*>(&(*it)) };
    tree_.emplace(k, it);
}

// Remove the block's entry from tree_ under key <blockUpper, address(block)> if present.
template <class Key>
void D1<Key>::delete_node_in_tree(double blockUpper, BlockIt it) {
    if (is_sentinel(it)) return;
    TauKey k{ blockUpper, static_cast<const void*>(&(*it)) };
    auto p = tree_.find(k);
    if (p != tree_.end()) tree_.erase(p);
}

// Remove up to 'count' nodes starting from the first blocks (front-to-back order).
// Deletes empty blocks as it goes and updates tree_ accordingly.
// Returns {taken, next_bound} where:
//  - taken: the list of removed nodes (in-order).
//  - next_bound: if there are remaining items, the minimum value in the next non-empty block;
//                otherwise, the maximum value among 'taken'; if nothing taken, +∞.
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
            const bool is_last = (std::next(bIt) == blocks_.end());
            if (is_last) {
                ++bIt;
            }
            else {
                delete_node_in_tree(bIt->blockUpper, bIt);
                bIt = blocks_.erase(bIt);
                
            }
        }
        else {
            ++bIt;
        }
    }

    bool found_non_empty = false;
    for (const auto& blk : blocks_) {
        if (!blk.items.empty()) {
            const auto minIt = std::min_element(
                blk.items.begin(), blk.items.end(),
                [](const Node& a, const Node& b) { return a.value < b.value; }
            );
            second_val = minIt->value;
            found_non_empty = true;
            break;
        }
    }
    if (!found_non_empty && !out.empty()) {
        const auto it = std::max_element(
            out.begin(), out.end(),
            [](const Node& a, const Node& b) { return a.value < b.value; }
        );
        second_val = it->value;
    }

    return { std::move(out), second_val };
}

// ================= BFPRT (Median-of-Medians) =================
// Internal BFPRT (median-of-medians) selection on an array of item iterators:
//  - Groups of 5 → medians → recursively select pivot median.
//  - Three-way partition around pivot; recurse into the bucket containing k.
// Guarantees linear-time selection for worst case; used by split().
template <class Key>
typename D1<Key>::ItemIt
D1<Key>::bfprt_select_(std::vector<ItemIt>& a, std::size_t k) {
    assert(!a.empty());
    assert(k < a.size());
    return bfprt_select_(a, 0, a.size(), k);
}

template <class Key>
typename D1<Key>::ItemIt
D1<Key>::bfprt_select_(std::vector<ItemIt>& a,
    std::size_t l, std::size_t r, std::size_t k)
{
    auto keyOf = [](const ItemIt& it) { return it->value; };
    const std::size_t len = r - l;


    if (len <= 16) {
        std::sort(a.begin() + l, a.begin() + r,
            [&](const ItemIt& x, const ItemIt& y) { return keyOf(x) < keyOf(y); });
        return a[l + k];
    }


    std::vector<ItemIt> meds; meds.reserve((len + 4) / 5);
    for (std::size_t i = l; i < r; i += 5) {
        std::size_t rr = std::min(r, i + 5);
        std::sort(a.begin() + i, a.begin() + rr,
            [&](const ItemIt& x, const ItemIt& y) { return keyOf(x) < keyOf(y); });
        std::size_t seg = rr - i;
        meds.push_back(a[i + (seg - 1) / 2]);
    }


    const ItemIt pivotIt = bfprt_select_(meds, 0, meds.size(), meds.size() / 2);
    const double pivot = keyOf(pivotIt);
    
    auto lessEnd = std::partition(a.begin() + l, a.begin() + r,
        [&](const ItemIt& x) { return keyOf(x) < pivot; });
    auto equalEnd = std::partition(lessEnd, a.begin() + r,
        [&](const ItemIt& x) { return !(pivot < keyOf(x)); }); 

    const std::size_t L = static_cast<std::size_t>(lessEnd - (a.begin() + l));
    const std::size_t E = static_cast<std::size_t>(equalEnd - lessEnd);

    if (k < L)                 return bfprt_select_(a, l, l + L, k);
    else if (k < L + E)        return *(lessEnd + (k - L));
    else                       return bfprt_select_(a, l + L + E, r, k - L - E);
}

