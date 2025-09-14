#pragma once
#include <vector>
#include <cstddef>
#include <string>
#include <functional>

namespace sssp {

    template<class Key>
    struct RelaxResult {
        std::vector<Key> W_union;
        std::vector<Key> decreased_keys;
    };

    template<class Key>
    RelaxResult<Key> relax_k_steps(
        const std::vector<std::vector<std::pair<Key, double>>>& adj, 
        std::vector<double>& db,                                     
        std::vector<Key> S,                                          
        double B,                                                    
        std::size_t K                                                
    );

    RelaxResult<std::string> relax_k_steps(
        const std::vector<std::vector<std::pair<std::string, double>>>& adj,
        std::vector<double>& db,
        std::vector<std::string> S,
        double B,
        std::size_t K,
        const std::function<std::size_t(const std::string&)>& index_of  // name -> index
    );

} // namespace sssp
