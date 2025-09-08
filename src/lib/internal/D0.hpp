#pragma once
#include <list>
#include <vector>
#include <cstddef>
#include <cassert>
#include <utility>
#include <algorithm>      
#include "ds_common.hpp"  

template <class Key>
class D0 {
public:
    using KV = ds::KV<Key>;
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



    BlockIt batchPrepend(std::list<KV>&& items) {
         if (items.empty()) return blocks_.end();
    
         std::list<Block> tmp;  
    
         while (!items.empty()) {
             Block b;
    
             const std::size_t take = std::min<std::size_t>(M_, items.size());
             auto it = items.begin();
             for (std::size_t i = 0; i < take; ++i) ++it;
    
             b.items.splice(b.items.end(), items, items.begin(), it);
    
             b.recompute_upper();
             b.recompute_lower();
    
             tmp.emplace_back(std::move(b));
             size_ += take;
         }
    
         blocks_.splice(blocks_.begin(), tmp);
    
         return blocks_.begin(); 
     }


    std::pair<std::list<KV>, std::size_t> pull() {
        return pull_n_(M_);
    }
   

private:
    std::size_t M_;
    BlockList   blocks_;
    std::size_t size_ = 0;



    std::pair<std::list<KV>, std::size_t> pull_n_(std::size_t n) {
        std::list<KV> out;
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
            else {
                b.recompute_upper();
                b.recompute_lower();
            }
        }

        return { std::move(out), remaining }; 
    }
};