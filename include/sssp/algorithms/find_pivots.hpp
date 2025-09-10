// include/sssp/algorithms/find_pivots.hpp
#pragma once
#include <vector>
#include <utility>     // std::pair
#include <string>
#include <functional>  // std::function

namespace sssp {


    template<class Key>
    struct PivotsResult {
        std::vector<Key> P;
        std::vector<Key> W;
    };

    template<class Key>
    PivotsResult<Key> find_pivots(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
        std::vector<Key> S,
        double B,
        std::size_t K);


    PivotsResult<std::string> find_pivots(
        const std::vector<std::vector<std::pair<std::string, double>>>& adj,
        std::vector<double>& db,
        std::vector<std::string> S,
        double B,
        std::size_t K,
        const std::function<std::size_t(const std::string&)>& index_of);

} // namespace sssp
