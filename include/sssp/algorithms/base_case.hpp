// sssp/algorithms/base_case.hpp
#pragma once
#include <vector>
#include <queue>
#include <utility>
#include <limits>
#include <functional>
#include <cassert>
#include "sssp/algorithms/types.hpp";
namespace sssp {
	template<class Key, class IndexOf>
	inline BMSSPResult<Key> base_case(
		double B,//חסם
		const std::vector<Key>& S,//קבוצת המקורות
		const AdjList<Key>& adj,//ייצוג של רשימת שכנויות - הגרף....
		std::vector<double>& db,//אומדני המרחק הנוכחיים  
		IndexOf index_of,
		std::size_t k//סף עצירה
	) {
		assert(S.size() == 1 && "BaseCase requires S to be a singleton");
		assert(k > 0 && "k must be positive");

		const std::size_t n = adj.size();
		assert(db.size() == n && "db.size() must equal adj.size()");

		const Key x = S.front(); // מוציאים את המקור היחיד x מתוך S
		const std::size_t ix = index_of(x); //ממירים את Key של x לאינדקס מספרי לתוך db/adj

		// U0 ? S
		std::vector<Key> U0;// צמתים ש"נשלפו/הושלמו" -יוצרים קבוצה
		U0.reserve(k + 1);//עד כמה פריטים יהיה ב U0
		U0.push_back(x);//מאתחלים את קבוצת הצמתים עם X
		std::vector<char> inU0(n, 0);//שלא ייכנס פעמייים, מערך בוליאני קטן לבדוק מהר אם צומת כבר נכנס ל-U0
		inU0[ix] = 1;//אם זה כבר בפנים מסומן 1




		//ערימת מינימום לפי מרחק 
		using PQItem = std::pair<double, Key>;//מרחק, צומת (מיצג כרגע כDOUBLE)
		auto cmp = [](const PQItem& a, const PQItem& b) {//פונ שמקבלת שתי צמתים ומשווה
			return a.first > b.first; // min-heap
			};

		std::priority_queue<PQItem, std::vector<PQItem>, decltype(cmp)> H(cmp);

		// initialize heap with ?x, db[x]?
		H.emplace(db[ix], x);

		// כדי לדלג על רשומות מיושנות (DecreaseKey אמור היה לצמצם),
		// נשווה בעת השליפה לערך העדכני ב-db.
		while (!H.empty() && U0.size() < k + 1) {
			const auto [du, u] = H.top(); H.pop();
			const std::size_t iu = index_of(u);
			if (du != db[iu]) continue; // רשומה מיושנת

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
				if (cand <= db[iv] && cand < B) {
					if (cand < db[iv]) {
						db[iv] = cand; 
					}
					// Insert / DecreaseKey: דוחפים רשומה חדשה. ישנות ייפסלו בשליפה.
					H.emplace(db[iv], v);
				}
			}
		}

		BMSSPResult<Key> out;

		// אם |U0| ? k ? (B' = B, U = U0)
		if (U0.size() <= k) {
			out.Bprime = B;
			out.U = std::move(U0);
			return out;
		}

		// אחרת: B' = max_{v?U0} db[v],  U = { v?U0 : db[v] < B' }
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