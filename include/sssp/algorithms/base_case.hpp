#pragma once
#include <vector>
#include <queue>
#include <utility>
#include <limits>
#include <functional>
#include <cassert>
#include "sssp/algorithms/types.hpp"

namespace sssp {
	template<class Key, class IndexOf>
	inline BMSSPResult<Key> base_case(
		double B,
		const std::vector<Key>& S,
		const AdjList<Key>& adj,
		std::vector<double>& db,
		IndexOf index_of,
		std::size_t k
	) {
		assert(S.size() == 1 && "BaseCase requires S to be a singleton");
		assert(k > 0 && "k must be positive");

		const std::size_t n = adj.size();
		assert(db.size() == n && "db.size() must equal adj.size()");

		const Key x = S.front(); 
		const std::size_t ix = index_of(x); 

		// U0 ? S
		std::vector<Key> U0;
		U0.reserve(k + 1);
		U0.push_back(x);
		std::vector<char> inU0(n, 0);
		inU0[ix] = 1;




		using PQItem = std::pair<double, Key>;
		auto cmp = [](const PQItem& a, const PQItem& b) {
			return a.first > b.first; // min-heap
			};

		std::priority_queue<PQItem, std::vector<PQItem>, decltype(cmp)> H(cmp);

		// initialize heap with ?x, db[x]?
		H.emplace(db[ix], x);

		while (!H.empty() && U0.size() < k + 1) {
			const auto [du, u] = H.top(); H.pop();
			const std::size_t iu = index_of(u);
			if (du != db[iu]) continue;

			// U0 ? U0 ? {u}
			if (!inU0[iu]) {
				inU0[iu] = 1;
				U0.push_back(u);
			}

			// for each edge (u,v):
			for (const auto& e : adj[iu]) {
				const Key& v = e.first;
				const double wuv = e.second;
				const std::size_t iv = index_of(v);

				const double cand = du + wuv;

				// if db[u] + wuv ? db[v] and db[u] + wuv < B
				if (cand < db[iv] && cand < B) {
					if (cand < db[iv]) {
						db[iv] = cand;
					}
					H.emplace(db[iv], v);
				}
			}
		}

		BMSSPResult<Key> out;

		// |U0| ? k ? (B' = B, U = U0)
		if (U0.size() <= k) {
			out.Bprime = B;
			out.U = std::move(U0);
			return out;
		}

		//  B' = max_{v?U0} db[v],  U = { v?U0 : db[v] < B' }
		double Bprime = -std::numeric_limits<double>::infinity();
		for (const Key& v : U0) {
			const std::size_t iv = index_of(v);
			if (db[iv] > Bprime) Bprime = db[iv];
		}

		out.Bprime = Bprime;
		out.U.reserve(U0.size());
		for (const Key& v : U0) {
			const std::size_t iv = index_of(v);
			if (db[iv] < Bprime) out.U.push_back(v);
		}
		return out;
	}

} // namespace sssp