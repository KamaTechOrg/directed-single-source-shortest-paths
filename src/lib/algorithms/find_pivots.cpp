// src/sssp/algorithms/find_pivots.cpp
#include "sssp/algorithms/find_pivots.hpp"
#include "sssp/algorithms/relax.hpp"

#include <vector>
#include <utility>
#include <functional>
#include <type_traits>
#include <cstddef>
#include <cmath>
#include <algorithm>

namespace sssp {

    namespace {
        // Convert an integral key (int, char, etc.) into a safe index (size_t).
        // For 'char', we cast to unsigned first to avoid negative values.
        // Used when db[] and adj[] are indexed by integer keys directly.
        template<class Key>
        inline std::size_t to_index_integral(const Key& k) {
            static_assert(std::is_integral<Key>::value, "Key must be integral");
            if constexpr (std::is_same_v<Key, char>) {
                return static_cast<std::size_t>(static_cast<unsigned char>(k));
            }
            else {
                return static_cast<std::size_t>(k);
            }
        }

        // Check if an edge is "tight": db[v] == db[u] + w(u,v).
        // Because of floating-point errors we cannot use exact equality.
        // Instead, we check if the difference is very small compared to the scale.
        inline bool tight(double sum, double dv) {
            const double diff = std::abs(sum - dv);
            const double scale = 1.0 + std::max(std::abs(sum), std::abs(dv));
            return diff <= 1e-12 * scale;
        }

        // Count the size of the subtree of 'root_idx' in the forest (children graph).
        // Stop early if the count reaches K (we only care if size >= K).
        // Returns true if subtree size is at least K, otherwise false.
        inline bool subtree_at_least_K(std::size_t root_idx,
            const std::vector<std::vector<std::size_t>>& children,
            std::size_t K)
        {
            int cnt = 0;
            std::vector<std::size_t> st;
            st.push_back(root_idx);

            while (!st.empty() && cnt < static_cast<int>(K)) {
                auto x = st.back(); st.pop_back();
                ++cnt;
                for (auto y : children[x]) {
                    st.push_back(y);
                    if (cnt >= static_cast<int>(K)) break; // stop early
                }
            }
            return cnt >= static_cast<int>(K);
        }

    } // anonymous namespace

    // ======================
    //   Integral Key version
    // ======================
    template<class Key>
    PivotsResult<Key> find_pivots(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
        std::vector<Key> S,     // by value – הרלקסציה שלך צורכת בבטחה
        double B,
        std::size_t K)
    {
        static_assert(std::is_integral<Key>::value,
            "This overload expects an integral Key (e.g., int/char)");

        const std::size_t n = adj.size();
        const std::vector<Key> S0 = S; 

        RelaxResult<Key> rr = relax_k_steps(adj, db, std::move(S), B, K);
        const std::vector<Key>& W = rr.W_union;

        if (W.size() > K * S0.size()) {
            return PivotsResult<Key>{ S0, W };
        }

        std::vector<char> inW(n, 0), inS0(n, 0);
        for (const Key& v : W)  inW[to_index_integral(v)] = 1;
        for (const Key& u : S0) inS0[to_index_integral(u)] = 1;

        std::vector<std::size_t> pred_idx(n, (std::size_t)-1);
        std::vector<char>        pred_set(n, 0);

        for (const Key& uKey : W) {
            const std::size_t iu = to_index_integral(uKey);
            const double du = db[iu];
            for (const auto& e : adj[iu]) {
                const Key& vKey = e.first;
                const double wuv = e.second;
                const std::size_t iv = to_index_integral(vKey);
                if (!inW[iv]) continue;

                const double cand = du + wuv;
                if (tight(cand, db[iv])) {
                    if (!pred_set[iv] || iu < pred_idx[iv]) {
                        pred_set[iv] = 1;
                        pred_idx[iv] = iu;
                    }
                }
            }
        }

        std::vector<int> indeg(n, 0);
        std::vector<std::vector<std::size_t>> children(n);
        for (const Key& vKey : W) {
            const std::size_t iv = to_index_integral(vKey);
            const std::size_t iu = pred_idx[iv];
            if (iu != (std::size_t)-1 && inW[iu]) {
                children[iu].push_back(iv);
                ++indeg[iv];
            }
        }

        std::vector<Key> P;
        P.reserve(S0.size());
        for (const Key& uKey : S0) {
            const std::size_t iu = to_index_integral(uKey);
            if (inW[iu] && indeg[iu] == 0 && subtree_at_least_K(iu, children, K)) {
                P.push_back(uKey);
            }
        }

        return PivotsResult<Key>{ P, W };
    }

    // ======================
    //   std::string version
    // ======================
    PivotsResult<std::string> find_pivots(
        const std::vector<std::vector<std::pair<std::string, double>>>& adj,
        std::vector<double>& db,
        std::vector<std::string> S,  // by value
        double B,
        std::size_t K,
        const std::function<std::size_t(const std::string&)>& index_of)
    {
        const std::size_t n = adj.size();
        const std::vector<std::string> S0 = S;

        RelaxResult<std::string> rr = relax_k_steps(adj, db, std::move(S), B, K, index_of);
        const std::vector<std::string>& W = rr.W_union;

        if (W.size() > K * S0.size()) {
            return PivotsResult<std::string>{ S0, W };
        }

        std::vector<char> inW(n, 0), inS0(n, 0);
        for (const auto& v : W)  inW[index_of(v)] = 1;
        for (const auto& u : S0) inS0[index_of(u)] = 1;

        std::vector<std::size_t> pred_idx(n, (std::size_t)-1);
        std::vector<char>        pred_set(n, 0);

        for (const auto& uKey : W) {
            const std::size_t iu = index_of(uKey);
            const double du = db[iu];
            for (const auto& e : adj[iu]) {
                const std::string& vKey = e.first;
                const double       wuv = e.second;
                const std::size_t  iv = index_of(vKey);
                if (!inW[iv]) continue;

                const double cand = du + wuv;
                if (tight(cand, db[iv])) {
                    if (!pred_set[iv] || iu < pred_idx[iv]) {
                        pred_set[iv] = 1;
                        pred_idx[iv] = iu;
                    }
                }
            }
        }

        std::vector<int> indeg(n, 0);
        std::vector<std::vector<std::size_t>> children(n);
        for (const auto& vKey : W) {
            const std::size_t iv = index_of(vKey);
            const std::size_t iu = pred_idx[iv];
            if (iu != (std::size_t)-1 && inW[iu]) {
                children[iu].push_back(iv);
                ++indeg[iv];
            }
        }

        std::vector<std::string> P;
        P.reserve(S0.size());
        for (const auto& uKey : S0) {
            const std::size_t iu = index_of(uKey);
            if (inW[iu] && indeg[iu] == 0 && subtree_at_least_K(iu, children, K)) {
                P.push_back(uKey);
            }
        }

        return PivotsResult<std::string>{ P, W };
    }

    // ======================
    // Explicit Instantiation
    // ======================
    template PivotsResult<int> find_pivots<int>(
        const std::vector<std::vector<std::pair<int, double>>>&,
        std::vector<double>&,
        std::vector<int>,
        double,
        std::size_t);

    template PivotsResult<char> find_pivots<char>(
        const std::vector<std::vector<std::pair<char, double>>>&,
        std::vector<double>&,
        std::vector<char>,
        double,
        std::size_t);

} // namespace sssp
