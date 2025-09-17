#pragma once
#include <vector>
#include <utility>

namespace sssp {

    template<class Key>
    using AdjList = std::vector<std::vector<std::pair<Key, double>>>;

    template<class Key>
    struct PivotsResult {
        std::vector<Key> P;
        std::vector<Key> W;
    };

    template<class Key>
    struct BMSSPResult {
        double Bprime{};
        std::vector<Key> U;
    };

} // namespace sssp
