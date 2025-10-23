#pragma once
#include <type_traits>
#include <cstddef>
#include <unordered_map>
#include <string>

namespace sssp {
	namespace util {



		template<class Key>
		inline auto make_integral_indexer() {
			static_assert(std::is_integral_v<Key>, "Integral keys only");
			//	if constexpr (std::is_same_v<Key, char>) {
			//		return [](char c) -> std::size_t {
			//			return static_cast<std::size_t>(static_cast<unsigned char>(c));
			//			};
			//	}
			//	else {
			//		return [](Key k) -> std::size_t { return static_cast<std::size_t>(k); };
			//	}
			//}
			if constexpr (std::is_signed_v<Key>) {
				return [](Key x) -> std::size_t {
					using U = std::make_unsigned_t<Key>;
					return static_cast<std::size_t>(static_cast<U>(x));
					};
			}
			else {
				return [](Key x) -> std::size_t {
					return static_cast<std::size_t>(x);
					};
			}
		}
	
			template<class Key>
			inline auto make_map_indexer(const std::unordered_map<Key, std::size_t>&m) {
				return [&m](const Key& k) -> std::size_t { return m.at(k); };
			}
		// namespace util
	} // namespace sssp::util
}