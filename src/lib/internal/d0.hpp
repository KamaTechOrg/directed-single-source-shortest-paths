#pragma once
#include <list>
#include <vector>
#include <cstddef>
#include <cassert>
#include <utility>
#include <algorithm>  
#include <limits>
#include "ds_common.hpp"  

template <class Key>
class D0 {
public:
    using Node = ds::Node<Key>;
    using Block = ds::Block<Key>;
    using BlockList = std::list<Block>;
    using BlockIt = typename BlockList::iterator;
    using CBlockIt = typename BlockList::const_iterator;

    explicit D0(std::size_t M) : M_(M) {
        assert(M_ > 0 && "D0 requires M > 0");
    }

    std::size_t M()     const noexcept { return M_; }
    std::size_t size()  const noexcept { return size_; }
    bool        empty() const noexcept { return size_ == 0; }

    BlockList& blocks()       noexcept { return blocks_; }
    const BlockList& blocks() const noexcept { return blocks_; }

    BlockIt  begin()        noexcept { return blocks_.begin(); }
    BlockIt  end()          noexcept { return blocks_.end(); }
    CBlockIt begin()  const noexcept { return blocks_.begin(); }
    CBlockIt end()    const noexcept { return blocks_.end(); }
    CBlockIt cbegin() const noexcept { return blocks_.cbegin(); }
    CBlockIt cend()   const noexcept { return blocks_.cend(); }



    BlockIt batchPrepend(std::list<Node>&& items) {
         if (items.empty()) return blocks_.end();
    
         std::list<Block> tmp;  
    
         while (!items.empty()) {
             Block b;
    
             const std::size_t take = std::min<std::size_t>(M_, items.size());
             auto it = items.begin();
             for (std::size_t i = 0; i < take; ++i) ++it;
    
             b.items.splice(b.items.end(), items, items.begin(), it);
    
    
             tmp.emplace_back(std::move(b));
             size_ += take;
         }
    
         blocks_.splice(blocks_.begin(), tmp);
    
         return blocks_.begin(); 
     }


    std::pair<std::list<Node>, std::size_t> pull(double* second_val, std::size_t n) {
        if (second_val) {
            *second_val = std::numeric_limits<double>::infinity();
        }

        std::list<Node> out;
        std::size_t remaining = n;

        while (remaining > 0 && !blocks_.empty()) {
            while (!blocks_.empty() && blocks_.front().items.empty())
                blocks_.pop_front();
            if (blocks_.empty()) break;

            Block& b = blocks_.front();
            const std::size_t can_take = std::min<std::size_t>(remaining, b.items.size());

            if (can_take == b.items.size()) {
                out.splice(out.end(), b.items);
            }
            else {
                auto it = b.items.begin();
                for (std::size_t i = 0; i < can_take; ++i) ++it;
                out.splice(out.end(), b.items, b.items.begin(), it);
            }

            remaining -= can_take;
            size_ -= can_take;

            if (b.items.empty()) {
                blocks_.pop_front();
            }

        }


        if (second_val && remaining == 0) {
            Block* first_non_empty = nullptr;
            for (auto& blk : blocks_) {
                if (!blk.items.empty()) {
                    first_non_empty = &blk;
                    break;
                }
            }

            if (first_non_empty) {
                const auto& items = first_non_empty->items;
                const auto minIt = std::min_element(
                    items.begin(), items.end(),
                    [](const Node& a, const Node& b) { return a.value < b.value; }
                );
                if (minIt != items.end()) {
                    *second_val = minIt->value;
                }
            }
            else {
                if (!out.empty()) {
                    const auto maxIt = std::max_element(
                        out.begin(), out.end(),
                        [](const Node& a, const Node& b) { return a.value < b.value; }
                    );
                    *second_val = maxIt->value;
                }
            }
        }

        return { std::move(out), remaining };
    }
   

private:
    std::size_t M_;
    BlockList   blocks_;
    std::size_t size_ = 0;

};