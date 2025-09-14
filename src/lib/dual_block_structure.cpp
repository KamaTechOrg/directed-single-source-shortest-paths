#include "sssp/dual_block_structure.hpp"

#include "src/lib/internal/d0.hpp"
#include "src/lib/internal/d1.hpp"     
#include "src/lib/internal/hash.hpp"
#include <string>

template <class Key>
DualBlockStructure<Key>::DualBlockStructure(std::size_t M, std::size_t B, std::size_t expected_keys)
    : M_(M)
    , B_(B)
    , d0_(M)                                  
    , d1_(M, static_cast<double>(B))          
    , hash_(expected_keys)                    
{
    assert(M_ > 0 && "DualBlockStructure: M must be > 0");
}

template <class Key>
void DualBlockStructure<Key>::initialize(std::size_t M, std::size_t B, std::size_t expected_keys)
{
    assert(M > 0 && "DualBlockStructure::initialize: M must be > 0");

    M_ = M;
    B_ = B;

    d0_ = D0<Key>(M);
    d1_ = D1<Key>(M, static_cast<double>(B));
    hash_ = Hash<Key>(expected_keys);
}

template <class Key>
void DualBlockStructure<Key>::batch_prepend(KVList&& items) {
    d0_.batchPrepend(std::move(items));
}

template <class Key>
std::pair<typename DualBlockStructure<Key>::KVList, double>
DualBlockStructure<Key>::pull() {
    KVList out;

    double bound = std::numeric_limits<double>::infinity();

    double d0_second = std::numeric_limits<double>::infinity();
    auto [from_d0, remaining] = d0_.pull(&d0_second, d0_.M());
    out.splice(out.end(), from_d0);

    if (remaining > 0) {
        d0_second = std::numeric_limits<double>::infinity();

        auto [from_d1, d1_second] = d1_.pull(remaining);
        out.splice(out.end(), from_d1);
        bound = d1_second; 
    }
    else {
        bound = d0_second;
    }

    return { std::move(out), bound };
}

template <class Key>
bool DualBlockStructure<Key>::insert(const Key& key, double value) {
    if (auto* h = hash_.get(key)) {
        double old_val = h->value();
        if (old_val == value) {
            return false; 
        }
        erase_existing_(*h);
        hash_.erase(key);
    }

    auto bit = d1_.choose_block_for_value(value);

    auto& items = bit->items;                       
    auto iit = items.insert(items.end(), Node{ key, value });

    ds::Handle<Key> nh;
    nh.blockIt = bit;
    nh.itemIt = iit;

    hash_.insert(key, nh);
    return true;
}

template <class Key>
void DualBlockStructure<Key>::erase_existing_(const ds::Handle<Key>& h) {
   d1_.delete_item(h.blockIt, h.itemIt);
}


template class DualBlockStructure<int>;
template class DualBlockStructure<char>;
template class DualBlockStructure<std::string>;

