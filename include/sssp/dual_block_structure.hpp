// dual_block_structure.hpp
enum class InsertResult { NoChange, InsertedNew, Decreased };

template<class Key>
class DualBlockStructure {
public:
    //insert new node
    InsertResult insert_or_decrease(const Key& k, double newVal);

private:
    // choose a block in d1 by search in the tree
    typename D1<Key>::BlockIt choose_block_for_value_in_d1(double v);

    D0<Key>   d0_;
    D1<Key>   d1_;
    Hash<Key> hash_;
};
