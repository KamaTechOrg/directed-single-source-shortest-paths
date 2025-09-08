#include "sssp/dual_block_structure.hpp"

#include "src/lib/internal/d0.hpp"
#include "src/lib/internal/d1.hpp"     
#include "src/lib/internal/hash.hpp"

template <class Key>
DualBlockStructure<Key>::DualBlockStructure(std::size_t M, std::size_t B, std::size_t expected_keys)
    : M_(M), B_(B), d0_(M), d1_(M, B), hash_(expected_keys)
{
    assert(M_ > 0);
}

template <class Key>
void DualBlockStructure<Key>::batch_prepend(KVList&& items) {
    d0_.batchPrepend(std::move(items));
}

template <class Key>
typename DualBlockStructure<Key>::KVList
DualBlockStructure<Key>::pull() {
    KVList out;
    auto [from_d0, deficit] = d0_.pull();   
    out.splice(out.end(), from_d0);

    if (deficit > 0) {
        KVList from_d1 = d1_.pull(deficit);
        out.splice(out.end(), from_d1);
    }
    return out; 
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
    auto iit = items.insert(items.end(), KV{ key, value });

    ds::Handle<Key> nh;
    nh.tier = ds::Tier::D1;
    nh.blockIt = bit;
    nh.itemIt = iit;

    hash_.insert(key, nh);
    return true;
}

template <class Key>
void DualBlockStructure<Key>::erase_existing_(const ds::Handle<Key>& h) {
    if (h.tier == ds::Tier::D0) {
     
        auto bit = h.blockIt;   
        auto iit = h.itemIt;    
        auto& bl = d0_.blocks();

        bit->items.erase(iit);
        if (bit->items.empty()) {
            bl.erase(bit);
        }
        else {
            bit->recompute_upper();
            bit->recompute_lower();
        }
    }
    else {
        d1_.delete_item(h.blockIt, h.itemIt);
    }
}


template class DualBlockStructure<int>;
template class DualBlockStructure<char>;

