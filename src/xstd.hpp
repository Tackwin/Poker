#pragma once

#include <type_traits>
#include <string>
#include <vector>
#include <atomic>
#include <functional>

namespace xstd {
	template<typename T>
        void type_id() {};
    
	using type_id_t = void(*)();
    
    
	template<class T>
        struct is_vector {
		using type = T;
		using contained = T;
		constexpr static bool value = false;
	};
    
	template<class T>
        struct is_vector<std::vector<T>> {
		using type = std::vector<T>;
		using contained_t = T;
		constexpr static bool value = true;
	};
    
	template< typename T>
        inline constexpr bool is_vector_v = is_vector<T>::value;
    
	template< typename T>
        using is_vector_contained = typename is_vector<T>::contained_t;
    
	template< typename T>
        using is_vector_t = typename is_vector<T>::type;
    
	template<typename C, typename P>
        [[nodiscard]] C filter(const C& c, const P& p) noexcept {
		C result;
		auto it = std::back_inserter(result);
		for (auto& x : c) if (p(x)) *it++ = x;
		return result;
	}
    
	struct error_t {
		size_t code{ 0 };
		std::string desc{ "" };
	};
    
	template<typename T, typename E>
        struct expected {
		expected(T t) noexcept : t(std::move(t)), is_error(false) {}
		expected(E e) noexcept : e(std::move(e)), is_error(true) {}
		~expected() noexcept {
			if (is_error) {
				if constexpr (std::is_class_v<E>) e.~E();
			}
			else if constexpr (std::is_class_v<T>) {
				t.~T();
			}
		}
		operator bool() noexcept { return !is_error; }
		explicit operator T() noexcept { return t; }
		explicit operator E() noexcept { return e; }
		T& value() noexcept { return t; }
		E& error() noexcept { return e; }
        
		T& operator->() noexcept { return t; }
		T& operator*() noexcept { return t; }
        private:
		union {
			E e;
			T t;
		};
		bool is_error{ false };
	};
    
	template<typename T>
        using std_expected = expected<T, size_t>;
    
	constexpr inline size_t hash(size_t h, const char* str) noexcept {
		return !*str ? 0 : hash((h << 5) + h + *str, str + 1);
	}
	constexpr inline size_t hash(const char* str) noexcept {
		return !str ? 0 : hash(5381, str);
	}
    
	template<typename T>
        constexpr inline size_t hash_combine(size_t seed, const T& v) noexcept {
		std::hash<T> h;
		seed ^= h(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
	constexpr inline size_t hash_combine(size_t a, size_t b) noexcept {
#pragma warning(push)
#pragma warning(disable: 4307)
		return a + 0x9e3779b9u + (b << 6) + (b >> 2);
#pragma warning(pop)
	}

	template<typename T, typename U> constexpr size_t offset_of(U T::*member) {
		return (char*)&((T*)nullptr->*member) - (char*)nullptr;
	}

	template<typename T, typename U>
	constexpr inline const T* find_member(
		const std::vector<T>& vec, size_t offset, U&& u
	) noexcept {
		for (auto& x : vec) {
			const char* byte = (const char*)& x;
			byte += offset;
			auto candidate = *(const std::decay_t<U>*)byte;
			if (candidate == u) return &x;
		}
		return nullptr;
	}

	inline std::vector<std::string> split(std::string_view str, std::string_view delim) noexcept {
		std::vector<std::string> result;

		size_t i = 0;
		for (; i < str.size(); i += delim.size()) {
			auto to = str.find_first_of(delim, i);
			result.push_back((std::string)str.substr(i, to - i));
			i = to;
			if (i == str.npos) break;
		}

		return result;
	}

	template<typename T, typename U>
	std::vector<U> vec_map(const std::vector<T>& vec, std::function<U(const T&)> f) noexcept {
		std::vector<U> us;
		us.reserve(vec.size());

		for (const auto& x : vec) us.push_back(f(x));

		return us;
	}

	template<typename T>
	constexpr T max(T a, T b) noexcept {
		return a > b ? a : b;
	}
	template<typename T>
	constexpr T min(T a, T b) noexcept {
		return a < b ? a : b;
	}

	template<typename T>
	constexpr T map_rp_to_01(T x) noexcept {
		return 1 - 1 / (1 + x);
	}

	template<typename T>
	inline void reorder(std::vector<T>& v, std::vector<size_t>& indices) noexcept {
		size_t i, j, k;
		T t;
		for(i = 0; i < v.size(); i++){
			if(i != indices[i]){
				t = v[i];
				k = i;
				while(i != (j = indices[k])){
				// every move places a value in it's final location
					v[k] = v[j];
					indices[k] = k;
					k = j;
				}
				v[k] = t;
				indices[k] = k;
			}
		}
	}
}
