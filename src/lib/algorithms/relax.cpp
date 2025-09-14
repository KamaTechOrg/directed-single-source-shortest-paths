#include "sssp/algorithms/relax.hpp"
#include <cassert>
#include <type_traits>
#include <cstddef>
#include <vector>
#include <string>
#include <utility>
#include <functional>

namespace sssp {

    template<class Key>
    static inline std::size_t to_index(Key k) {
        static_assert(std::is_integral<Key>::value, "Key must be integral for this overload");
        if constexpr (std::is_same_v<Key, char>) {
            return static_cast<std::size_t>(static_cast<unsigned char>(k));
        }
        else {
            return static_cast<std::size_t>(k);
        }
    }

    template<class Key>
    RelaxResult<Key> relax_k_steps(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
        std::vector<Key> S,
        double B,
        std::size_t K)
    {
        static_assert(std::is_integral<Key>::value, "This template overload is for integral Key (int/char)");
        const std::size_t n = adj.size();
        assert(db.size() == n && "db.size() must equal adj.size()");

        RelaxResult<Key> out;
        out.W_union.reserve(n);

        std::vector<char> inW(n, 0);
        std::vector<char> decMarked(n, 0);          

        auto push_union = [&](const Key& vKey) {
            std::size_t iv = to_index(vKey);
            if (!inW[iv]) { inW[iv] = 1; out.W_union.push_back(vKey); }
            };

        // W0 = S
        for (const Key& u : S) push_union(u);

        std::vector<Key> Wi_prev = std::move(S);
        std::vector<Key> Wi;
        Wi.reserve(256);

        for (std::size_t step = 1; step <= K; ++step) {
            Wi.clear();

            for (const Key& uKey : Wi_prev) {
                std::size_t iu = to_index(uKey);
                const double du = db[iu];

                for (const auto& edge : adj[iu]) {
                    const Key& vKey = edge.first;
                    const double wuv = edge.second;

                    std::size_t iv = to_index(vKey);
                    const double cand = du + wuv;

                    if (cand <= db[iv]) {
                        if (cand < db[iv]) {
                            db[iv] = cand;
                            if (!decMarked[iv]) {          
                                decMarked[iv] = 1;
                                out.decreased_keys.push_back(vKey);
                            }
                        }
                        if (cand < B && !inW[iv]) {        
                            Wi.push_back(vKey);
                        }
                    }
                }
            }

            for (const Key& vKey : Wi) push_union(vKey);
            if (Wi.empty()) break;
            Wi_prev.swap(Wi);
        }

        return out;
    }

    // ===== std::string index_of =====
    RelaxResult<std::string> relax_k_steps(
        const std::vector<std::vector<std::pair<std::string, double>>>& adj,
        std::vector<double>& db,
        std::vector<std::string> S,
        double B,
        std::size_t K,
        const std::function<std::size_t(const std::string&)>& index_of)
    {
        const std::size_t n = adj.size();
        assert(db.size() == n && "db.size() must equal adj.size()");

        RelaxResult<std::string> out;
        out.W_union.reserve(n);

        std::vector<char> inW(n, 0);
        std::vector<char> decMarked(n, 0);          

        auto push_union = [&](const std::string& vKey) {
            std::size_t iv = index_of(vKey);
            if (!inW[iv]) { inW[iv] = 1; out.W_union.push_back(vKey); }
            };

        // W0 = S
        for (const auto& u : S) push_union(u);

        std::vector<std::string> Wi_prev = std::move(S);
        std::vector<std::string> Wi;
        Wi.reserve(256);

        for (std::size_t step = 1; step <= K; ++step) {
            Wi.clear();

            for (const auto& uKey : Wi_prev) {
                std::size_t iu = index_of(uKey);
                const double du = db[iu];

                for (const auto& edge : adj[iu]) {
                    const std::string& vKey = edge.first;
                    const double wuv = edge.second;

                    std::size_t iv = index_of(vKey);
                    const double cand = du + wuv;

                    if (cand <= db[iv]) {
                        if (cand < db[iv]) {
                            db[iv] = cand;
                            if (!decMarked[iv]) {          
                                decMarked[iv] = 1;
                                out.decreased_keys.push_back(vKey);
                            }
                        }
                        if (cand < B && !inW[iv]) {        
                            Wi.push_back(vKey);
                        }
                    }
                }
            }

            for (const auto& vKey : Wi) push_union(vKey);
            if (Wi.empty()) break;
            Wi_prev.swap(Wi);
        }

        return out;
    }

    // Explicit instantiation 
    template RelaxResult<int> relax_k_steps<int>(const std::vector<std::vector<std::pair<int, double>>>&, std::vector<double>&, std::vector<int>, double, std::size_t);
    template RelaxResult<char> relax_k_steps<char>(const std::vector<std::vector<std::pair<char, double>>>&, std::vector<double>&, std::vector<char>, double, std::size_t);

} // namespace sssp
