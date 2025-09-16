#pragma once
#include <type_traits>
#include <string>
#include <cstddef>
#include <functional>

namespace sssp {
	namespace util {

		template<class Key>
		inline std::size_t key_to_index(const Key& k) {
			static_assert(std::is_integral_v<Key>, "Key must be integral here");
			if constexpr (std::is_same_v<Key, char>) {
				return static_cast<std::size_t>(static_cast<unsigned char>(k));
			}
			else {
				return static_cast<std::size_t>(k);
			}
		}

		inline std::size_t key_to_index(const std::string& s,
			const std::function<std::size_t(const std::string&)>& index_of) {
			return index_of(s);
		}
	}
} // namespace sssp::util
