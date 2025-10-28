    //#pragma once
    //#include <vector>
    //#include <utility>
    //#include <cstddef>
    //#include <cmath>
    //#include <algorithm>
    //
    //#include "sssp/algorithms/relax.hpp"
    //#include "sssp/algorithms/types.hpp"
    //namespace sssp {
    //
    //
    //    namespace detail {
    //        inline bool tight(double sum, double dv) {
    //            const double diff = std::abs(sum - dv);
    //            const double scale = 1.0 + std::max(std::abs(sum), std::abs(dv));
    //            return diff <= 1e-12 * scale;
    //        }
    //        inline bool subtree_at_least_K(std::size_t root_idx,
    //            const std::vector<std::vector<std::size_t>>& children,
    //            std::size_t K)
    //        {
    //            std::size_t cnt = 0;
    //            std::vector<std::size_t> st; st.push_back(root_idx);
    //            while (!st.empty() && cnt < K) {
    //                auto x = st.back(); st.pop_back();
    //                ++cnt;
    //                for (auto y : children[x]) {
    //                    st.push_back(y);
    //                    if (cnt >= K) break;
    //                }
    //            }
    //            return cnt >= K;
    //        }
    //    } // namespace detail
    //
    //    template<class Key, class IndexOf>
    //    inline PivotsResult<Key> find_pivots(
    //        const std::vector<std::vector<std::pair<Key, double>>>& adj,
    //        std::vector<double>& db,
    //        std::vector<Key> S,
    //        double B,
    //        std::size_t K,
    //        IndexOf vertex_index_fn)
    //    {
    //        const std::size_t n = adj.size();
    //        const std::vector<Key> S0 = S; 
    //
    //        auto rr = relax_k_steps<Key>(adj, db, S, B, K, vertex_index_fn);
    //        const auto& W = rr.W_union;
    //
    //        if (W.size() > K * S0.size()) {
    //            return PivotsResult<Key>{ S0, W };
    //        }
    //
    //        std::vector<char> inW(n, 0), inS0(n, 0);
    //        for (const Key& v : W)  inW[vertex_index_fn(v)] = 1;
    //        for (const Key& u : S0) inS0[vertex_index_fn(u)] = 1;
    //
    //        std::vector<std::size_t> pred_idx(n, (std::size_t)-1);
    //        std::vector<char>        pred_set(n, 0);
    //
    //        for (const Key& uKey : W) {
    //            const std::size_t iu = vertex_index_fn(uKey);
    //            const double du = db[iu];
    //            for (const auto& [vKey, wuv] : adj[iu]) {
    //                const std::size_t iv = vertex_index_fn(vKey);
    //                if (!inW[iv]) continue;
    //
    //                const double cand = du + wuv;
    //                if (detail::tight(cand, db[iv])) {
    //                    if (!pred_set[iv] || iu < pred_idx[iv]) {
    //                        pred_set[iv] = 1;
    //                        pred_idx[iv] = iu;
    //                    }
    //                }
    //            }
    //        }
    //
    //        std::vector<int> indeg(n, 0);
    //        std::vector<std::vector<std::size_t>> children(n);
    //        for (const Key& vKey : W) {
    //            const std::size_t iv = vertex_index_fn(vKey);
    //            const std::size_t iu = pred_idx[iv];
    //            if (iu != (std::size_t)-1 && inW[iu]) {
    //                children[iu].push_back(iv);
    //                ++indeg[iv];
    //            }
    //        }
    //
    //        std::vector<Key> P;
    //        P.reserve(S0.size());
    //        for (const Key& uKey : S0) {
    //            const std::size_t iu = vertex_index_fn(uKey);
    //            if (inW[iu] && indeg[iu] == 0 && detail::subtree_at_least_K(iu, children, K)) {
    //                P.push_back(uKey);
    //            }
    //        }
    //
    //        return PivotsResult<Key>{ P, W };
    //    }
    //
    //} // namespace sssp





////אחרי שיפור אחד
//    #pragma once
//    #include <vector>
//    #include <utility>
//    #include <cstddef>
//    #include <cmath>
//    #include <algorithm>
//    #include <limits>
//    #include <cstdint>
//
//    #include "sssp/algorithms/relax.hpp"
//    #include "sssp/algorithms/types.hpp"
//
//    namespace sssp {
//
//        namespace detail {
//            inline bool tight(double sum, double dv) {
//                const double diff = std::abs(sum - dv);
//                const double a = std::abs(sum);
//                const double b = std::abs(dv);
//                const double scale = 1.0 + (a > b ? a : b);
//                return diff <= 1e-12 * scale;
//            }
//        } // namespace detail
//
//        // מיישם בחירת הורה + "קילוף עלים" לחישוב גודל תת־עץ עד K, ללא בניית children דו-ממדי.
//        template<class Key, class IndexOf>
//        inline PivotsResult<Key> find_pivots(
//            const std::vector<std::vector<std::pair<Key, double>>>& adj,
//            std::vector<double>& db,
//            std::vector<Key> S,
//            double B,
//            std::size_t K,
//            IndexOf vertex_index_fn)
//        {
//            const std::size_t n = adj.size();
//            const std::vector<Key> S0 = S;
//
//            // --- מריצים הרפיה K צעדים — משתמש בגרסה האופטימית מהקובץ השני ---
//            // להקטנת הקצאות/איפוסים, נשתמש גם כאן בבאפרים ממוחזרים.
//            static thread_local std::vector<std::uint8_t> t_inW;
//            static thread_local std::vector<std::uint8_t> t_inS0;
//            static thread_local std::vector<std::uint8_t> t_predSet;
//            static thread_local std::vector<std::size_t>  t_parent;     // pred_idx
//            static thread_local std::vector<int>          t_indeg;      // #children
//            static thread_local std::vector<int>          t_sizeCapK;   // גודל תת-עץ עד K
//            static thread_local std::vector<std::size_t>  t_queue;      // תור לקילוף עלים
//
//            if (t_inW.size() != n) {
//                t_inW.assign(n, 0);
//                t_inS0.assign(n, 0);
//                t_predSet.assign(n, 0);
//                t_parent.assign(n, (std::size_t)-1);
//                t_indeg.assign(n, 0);
//                t_sizeCapK.assign(n, 0);
//                t_queue.clear();
//            }
//
//            // קריאה להרפיה (עם מחזור פנימי שלה)
//            auto rr = relax_k_steps<Key>(adj, db, S, B, K, vertex_index_fn);
//            const auto& W = rr.W_union;
//
//            // תנאי עצירה: W גדול מדי — נשארים עם S0
//            if (W.size() > K * S0.size()) {
//                return PivotsResult<Key>{ S0, W };
//            }
//
//            // נסמן רק את איברי W ו-S0, ונאסוף את האינדקסים שלהם לניקוי מהיר
//            std::vector<std::size_t> w_idx; w_idx.reserve(W.size());
//            for (const Key& v : W) { auto iv = vertex_index_fn(v); if (!t_inW[iv]) { t_inW[iv] = 1; w_idx.push_back(iv); } }
//            for (const Key& u : S0) { t_inS0[vertex_index_fn(u)] = 1; }
//
//            // איפוס נקודתי של המקטעים שניגע בהם
//            for (auto iv : w_idx) {
//                t_parent[iv] = (std::size_t)-1;
//                t_predSet[iv] = 0;
//                t_indeg[iv] = 0;
//                t_sizeCapK[iv] = 0;
//            }
//
//            // מציאת הורה (parent) לכל v ב-W: קצה "tight" מה-u המינימלי (iu) שנכנס ל-v
//            for (const Key& uKey : W) {
//                const std::size_t iu = vertex_index_fn(uKey);
//                const double du = db[iu];
//
//                for (const auto& [vKey, wuv] : adj[iu]) {
//                    const std::size_t iv = vertex_index_fn(vKey);
//                    if (!t_inW[iv]) continue;
//
//                    const double cand = du + wuv;
//                    if (detail::tight(cand, db[iv])) {
//                        if (!t_predSet[iv] || iu < t_parent[iv]) {
//                            t_predSet[iv] = 1;
//                            t_parent[iv] = iu;
//                        }
//                    }
//                }
//            }
//
//            // חישוב indeg (#children) להורים, ואתחול גודל תת-עץ ל-1 לכל איבר ב-W
//            for (auto iv : w_idx) {
//                t_sizeCapK[iv] = 1;  // כל צומת סופר את עצמו
//            }
//            for (auto iv : w_idx) {
//                auto p = t_parent[iv];
//                if (p != (std::size_t)-1 && t_inW[p]) ++t_indeg[p];
//            }
//
//            // קילוף עלים: מתחילים מכל צומת ללא ילדים, מצטברים למעלה עד K
//            t_queue.clear();
//            for (auto iv : w_idx) if (t_indeg[iv] == 0) t_queue.push_back(iv);
//
//            for (std::size_t qi = 0; qi < t_queue.size(); ++qi) {
//                auto v = t_queue[qi];
//                auto p = t_parent[v];
//                if (p != (std::size_t)-1 && t_inW[p]) {
//                    // צבירה למעלה עם חסם K
//                    int nv = t_sizeCapK[v];
//                    int& sp = t_sizeCapK[p];
//                    sp = std::min<int>(static_cast<int>(K), sp + nv);
//                    if (--t_indeg[p] == 0) t_queue.push_back(p);
//                }
//            }
//
//            // בחירת פיבוטים: ב-S0 שהינם ב-W ושגודל תת-העץ שלהם >= K
//            std::vector<Key> P; P.reserve(S0.size());
//            for (const Key& uKey : S0) {
//                const std::size_t iu = vertex_index_fn(uKey);
//                if (t_inW[iu] && t_sizeCapK[iu] >= static_cast<int>(K)) {
//                    P.push_back(uKey);
//                }
//            }
//
//            // ניקוי נקודתי של הסימונים
//            for (auto iv : w_idx) { t_inW[iv] = 0; t_predSet[iv] = 0; }
//            for (const Key& u : S0) t_inS0[vertex_index_fn(u)] = 0;
//
//
//            return PivotsResult<Key>{ P, W };
//        }
//
//    } // namespace sssp











//אחרי שיפור נוסף
#pragma once
#include <vector>
#include <utility>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstdint>

#include "sssp/algorithms/relax.hpp"
#include "sssp/algorithms/types.hpp"

namespace sssp {

    namespace detail {
        // מהיר יותר ל-double: fabs/fmax, inline
        inline bool tight(double sum, double dv) {
            const double diff = std::fabs(sum - dv);
            const double scale = 1.0 + std::fmax(std::fabs(sum), std::fabs(dv));
            return diff <= 1e-12 * scale;
        }
    } // namespace detail

    // בחירת הורה "tight" + קילוף עלים לחישוב גודל תת־עץ עד K — ללא children דו־ממדי וללא מיונים.
    template<class Key, class IndexOf>
    inline PivotsResult<Key> find_pivots(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
        std::vector<Key> S,
        double B,
        std::size_t K,
        IndexOf vertex_index_fn)
    {
        const std::size_t n = adj.size();
        const std::vector<Key> S0 = S;

        // ====== באפרים ממוחזרים (thread_local) ======
        // סימונים: חבר ב-W, יש הורה "tight"
        static thread_local std::vector<std::uint8_t> t_inW;
        static thread_local std::vector<std::uint8_t> t_predSet;

        // הורה (אינדקס) לכל צומת ב-W; -1 אם אין
        static thread_local std::vector<std::size_t>  t_parent;

        // #children בגרף ה"הורות" (לצורך קילוף עלים)
        static thread_local std::vector<int>          t_indeg;

        // גודל תת־עץ מצומצם ל-K
        static thread_local std::vector<int>          t_sizeCapK;

        // תורי עבודה
        static thread_local std::vector<std::size_t>  t_queue;
        static thread_local std::vector<std::size_t>  w_idx;    // אינדקסים של צמתים ב-W

        if (t_inW.size() != n) {
            t_inW.assign(n, 0);
            t_predSet.assign(n, 0);
            t_parent.assign(n, (std::size_t)-1);
            t_indeg.assign(n, 0);
            t_sizeCapK.assign(n, 0);
            t_queue.clear();
            w_idx.clear();
        }

        // ====== K-Relax (גרסה שלך) ======
        // הערה: מומלץ לשלב בעתיד epoch-marking בתוך relax_k_steps כדי לצמצם עלות איפוסי db.
        auto rr = relax_k_steps<Key>(adj, db, S, B, K, vertex_index_fn);
        const auto& W = rr.W_union;

        // אם W גדול מדי — לפי ההגדרה שלכם: חוזרים עם S0 (אין פיבוטים חדשים)
        if (W.size() > K * S0.size()) {
            return PivotsResult<Key>{ S0, W };
        }

        // ====== סימון נקודתי של W ======
        w_idx.clear();
        w_idx.reserve(W.size());
        for (const Key& v : W) {
            const std::size_t iv = vertex_index_fn(v);
            if (!t_inW[iv]) {
                t_inW[iv] = 1;
                w_idx.push_back(iv);
            }
        }

        // איפוס נקודתי לשדות שנשתמש בהם רק עבור הצמתים ב-W
        for (std::size_t iv : w_idx) {
            t_parent[iv] = (std::size_t)-1;
            t_predSet[iv] = 0;
            t_indeg[iv] = 0;
            t_sizeCapK[iv] = 0;  // נציב ל-1 בהמשך
        }

        // ====== מציאת הורה "tight" לכל iv ∈ W ======
        // עוברים על כל u ∈ W, בודקים שכנים v ב-W, אם du + wuv "tight" ל-db[v] — u הוא הורה אפשרי של v.
        // שברירי ביצועים:
        //  - חישוב iu/du פעם אחת לכל u
        //  - דילוג מהיר אם v לא ב-W (t_inW[iv] == 0)
        for (const Key& uKey : W) {
            const std::size_t iu = vertex_index_fn(uKey);
            const double du = db[iu];

            const auto& row = adj[iu];
            for (const auto& edge : row) {
                const std::size_t iv = vertex_index_fn(edge.first);
                if (!t_inW[iv]) continue; // v לא ב-W → לא חלק מהעצים שלנו

                const double cand = du + edge.second;
                // cand "שווה" ל-db[iv] בטולרנס → קצה tight
                if (detail::tight(cand, db[iv])) {
                    // נשמור הורה יחיד: אם אין עדיין, או נשבור שוויון לפי אינדקס קטן יותר
                    if (!t_predSet[iv] || iu < t_parent[iv]) {
                        t_predSet[iv] = 1;
                        t_parent[iv] = iu;
                    }
                }
            }
        }

        // ====== קילוף עלים + צבירת גדלים עד K ======
        // אתחול: כל צומת סופר את עצמו
        for (std::size_t iv : w_idx) {
            t_sizeCapK[iv] = 1;
        }
        // חישוב מספר ילדים להורים
        for (std::size_t iv : w_idx) {
            const std::size_t p = t_parent[iv];
            if (p != (std::size_t)-1 && t_inW[p]) {
                ++t_indeg[p];
            }
        }

        // תור התחלה: כל מי שאין לו ילדים
        t_queue.clear();
        t_queue.reserve(w_idx.size());
        for (std::size_t iv : w_idx) {
            if (t_indeg[iv] == 0) t_queue.push_back(iv);
        }

        // קליפות: עלים → הורה, עד שאין ילדים. מצמצמים ל-K למניעת גדילה מיותרת.
        for (std::size_t qi = 0; qi < t_queue.size(); ++qi) {
            const std::size_t v = t_queue[qi];
            const std::size_t p = t_parent[v];
            if (p != (std::size_t)-1 && t_inW[p]) {
                int& sp = t_sizeCapK[p];
                const int nv = t_sizeCapK[v];
                const int sum = sp + nv;
                sp = (sum < static_cast<int>(K) ? sum : static_cast<int>(K));
                if (--t_indeg[p] == 0) {
                    t_queue.push_back(p);
                }
            }
        }

        // ====== בחירת פיבוטים: u ∈ S0 וגם u ∈ W וגם |subtree(u)| ≥ K ======
        std::vector<Key> P;
        P.reserve(S0.size());
        for (const Key& uKey : S0) {
            const std::size_t iu = vertex_index_fn(uKey);
            if (t_inW[iu] && t_sizeCapK[iu] >= static_cast<int>(K)) {
                P.push_back(uKey);
            }
        }

        // ====== ניקוי סימונים נקודתי ======
        for (std::size_t iv : w_idx) {
            t_inW[iv] = 0;
            t_predSet[iv] = 0;
            // t_parent/indeg/sizeCapK יאותחלו ממילא בריצה הבאה נקודתית
        }

        return PivotsResult<Key>{ P, W };
    }

} // namespace sssp
