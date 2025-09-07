// D0.hpp
#pragma once
#include <list>
#include <cstddef>
#include "ds_common.hpp"

namespace ds {

    template <class Key>
    class D0 {
    public:

        using KV = ds::KV<Key>;
        using Block = ds::Block<Key>;
        using BlockList = std::list<Block>;
        using BlockIt = typename BlockList::iterator;
        using CBlockIt = typename BlockList::const_iterator;


        explicit D0(std::size_t M) : M_(M) {}

        std::size_t block_capacity() const noexcept { return M_; }
        void set_block_capacity(std::size_t M) noexcept { M_ = M; }

        bool empty() const noexcept;
        std::size_t blocks_count() const noexcept;  //number of blocks
        std::size_t size() const noexcept; //number of items


        BlockIt       begin() noexcept;
        BlockIt       end() noexcept;
        CBlockIt      begin() const noexcept;
        CBlockIt      end()   const noexcept;
        CBlockIt      cbegin() const noexcept;
        CBlockIt      cend()   const noexcept;

        //Block& front();          //return it for the first block
		const Block& front() const;  //return it for the first block (const)

        Block pop_front_block(); // take a complete block
        std::list<KV> take_from_front(std::size_t k); //take partiual from block - k items

        
		BlockIt push_block_front(const Block& b); //add in the front a block (copy)
		BlockIt push_block_front(Block&& b); //add in the front a block (move)



        BlockIt batch_prepend(std::list<KV>&& items);


        /*BlockList& blocks()       noexcept;
        const BlockList& blocks() const noexcept;*/

    private:
        BlockList   blocks_;
        std::size_t total_size_ = 0; //all the items in the blockList
        std::size_t M_; 

        
        Block make_block_from_prefix(std::list<KV>& items); // create block with M items

        void bump_total_size(std::ptrdiff_t delta) noexcept; // update the count of items
    };

} // namespace ds
