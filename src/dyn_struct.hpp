#ifndef DYN_STRUCT_HPP
#define DYN_STRUCT_HPP

#include <any>
#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <optional>
#include <filesystem>
#include <unordered_map>
#pragma once

#include <type_traits>

template<typename T>
class ValuePtr {
public:

	constexpr ValuePtr() noexcept {}
	~ValuePtr() noexcept { if(ptr) delete ptr; }

	constexpr ValuePtr(T* ptr) noexcept : ptr(ptr) {}

	ValuePtr(const ValuePtr<T>& other) noexcept {
		if (!other.ptr) {
			ptr = other.ptr;
			return;
		}
		ptr = new T(*other.ptr);
	}

	template<
		typename U,
		typename = std::enable_if_t<std::is_base_of_v<T, U>, int>
	>
	constexpr ValuePtr(ValuePtr<U>&& other) noexcept : ptr((T*)other.ptr) {
		other.ptr = nullptr;
	};

	ValuePtr<T>& operator=(const ValuePtr<T>& other) noexcept {
		if (ptr) delete ptr;
		if (!other.ptr) {
			ptr = other.ptr;
			return *this;
		}
		ptr = new T(*other.ptr);
		return *this;
	}

	constexpr ValuePtr<T>& operator=(ValuePtr<T>&& other) noexcept {
		if (ptr) delete ptr;
		ptr = other.ptr;
		other.ptr = nullptr;
		return *this;
	}

	constexpr operator bool() const noexcept {
		return ptr != nullptr;
	}

	constexpr T& operator*() const noexcept {
		return *ptr;
	}
	constexpr T* operator->() const noexcept {
		return ptr;
	}

	constexpr T* get() const noexcept {
		return ptr;
	}
	[[nodiscard]]
	constexpr T* release() noexcept {
		auto temp = ptr;
		ptr = nullptr;
		return temp;
	}

private:
	template<typename U>
	friend class ValuePtr;

	T* ptr{ nullptr };
};
struct dyn_struct {
	using integer_t = long long int;
	using real_t = long double;
	using string_t = std::string;
	using boolean_t = bool;
	using structure_t = std::unordered_map<std::string, ValuePtr<dyn_struct>>;
	using array_t = std::vector<ValuePtr<dyn_struct>>;
	using null_t = std::nullptr_t;

	using variant =
		std::variant<integer_t, real_t, string_t, boolean_t, structure_t, array_t, null_t>;

	dyn_struct() = default;

	dyn_struct(const dyn_struct&) = default;
	dyn_struct(dyn_struct&&) = default;

	dyn_struct& operator=(const dyn_struct&) = default;
	dyn_struct& operator=(dyn_struct&&) = default;
	template<typename T>
	dyn_struct(std::initializer_list<T> list) noexcept {
		array_t array;

		for (auto& x : list) {
			array.push_back({ new dyn_struct(x) });
		}

		value = array;
	}

	dyn_struct(std::initializer_list<std::pair<std::string, dyn_struct>> list) noexcept;

	template<typename T> dyn_struct(T&& object) noexcept {
		if constexpr (std::is_same_v<std::decay_t<T>, dyn_struct>) {
			*this = std::forward<T>(object);
		}
		else {
			to_dyn_struct(*this, std::forward<T>(object));
		}
	}

	template<typename T> explicit operator T() const noexcept {
		T t;
		from_dyn_struct(*this, t);
		return t;
	}

	dyn_struct& operator[](std::string_view str) noexcept;
	dyn_struct& operator[](size_t idx) noexcept;

	const dyn_struct& operator[](std::string_view str) const noexcept;
	const dyn_struct& operator[](size_t idx) const noexcept;

	void push_back(const dyn_struct& v) noexcept;
	void pop_back() noexcept;

	dyn_struct* clone() noexcept;

	size_t type_tag{ 0 };
	variant value;
};

struct dyn_struct_array_iterator_tag {
	dyn_struct::array_t* it;
};
struct dyn_struct_const_array_iterator_tag {
	const dyn_struct::array_t* it;
};
struct dyn_struct_structure_iterator_tag {
	dyn_struct::structure_t* it;
};

struct dyn_struct_array_iterator {
	dyn_struct::array_t::iterator iterator;

	const dyn_struct& operator*() const noexcept;
	dyn_struct& operator*() noexcept;
	const dyn_struct& operator->() const noexcept;
	dyn_struct& operator->() noexcept;

	dyn_struct_array_iterator& operator++() noexcept;
	bool operator==(const dyn_struct_array_iterator& other) const noexcept;
	bool operator!=(const dyn_struct_array_iterator& other) const noexcept;
};
struct dyn_struct_const_array_iterator {
	dyn_struct::array_t::const_iterator iterator;

	const dyn_struct& operator*() const noexcept;
	const dyn_struct& operator->() const noexcept;

	dyn_struct_const_array_iterator& operator++() noexcept;
	bool operator==(const dyn_struct_const_array_iterator& other) const noexcept;
	bool operator!=(const dyn_struct_const_array_iterator& other) const noexcept;
};
struct dyn_struct_structure_iterator {
	dyn_struct::structure_t::iterator iterator;

	std::pair<std::string, const dyn_struct&> operator*() const noexcept;
	std::pair<std::string, dyn_struct&> operator*() noexcept;
	std::pair<std::string, const dyn_struct&> operator->() const noexcept;
	std::pair<std::string, dyn_struct&> operator->() noexcept;

	dyn_struct_structure_iterator& operator++() noexcept;
	bool operator==(const dyn_struct_structure_iterator& other) const noexcept;
	bool operator!=(const dyn_struct_structure_iterator& other) const noexcept;
};

namespace std {
	template<>
	struct iterator_traits<dyn_struct_array_iterator> {
		typedef dyn_struct value_type;
		typedef int difference_type;
		typedef value_type& reference;
		typedef value_type* pointer;
		typedef forward_iterator_tag iterator_category;
	};
	template<>
	struct iterator_traits<dyn_struct_structure_iterator> {
		typedef std::pair<std::string, dyn_struct&> value_type;
		typedef int difference_type;
		typedef value_type& reference;
		typedef value_type* pointer;
		typedef forward_iterator_tag iterator_category;
	};
};

#define X(x)\
extern void to_dyn_struct(dyn_struct&, const x&) noexcept;\
extern void from_dyn_struct(const dyn_struct&, x&) noexcept;

X(dyn_struct::integer_t)
X(dyn_struct::real_t)
X(dyn_struct::string_t)
X(dyn_struct::boolean_t)
X(dyn_struct::array_t)
X(dyn_struct::structure_t)
X(dyn_struct::null_t)
X(int)
X(long)
X(unsigned)
X(long unsigned)
X(long long unsigned)
X(short)
X(unsigned short)
X(float)
X(double)

#undef X
extern void to_dyn_struct(dyn_struct&, const char*) noexcept;

extern dyn_struct& get(std::string_view str, const dyn_struct& d_struct) noexcept;
extern dyn_struct&
set(std::string_view str, const dyn_struct& value, dyn_struct& to) noexcept;

extern bool has(const dyn_struct& d_struct, std::string_view key) noexcept;
extern const dyn_struct* at(const dyn_struct& d_struct, std::string_view key) noexcept;

extern std::string format(const dyn_struct& s) noexcept;
extern std::string format(const dyn_struct& s, std::string_view indent) noexcept;
extern std::string format(const dyn_struct& s, size_t space_indent) noexcept;

extern std::string format_to_json(const dyn_struct& s) noexcept;
extern std::string format_to_json(const dyn_struct& s, std::string_view indent) noexcept;
extern std::string format_to_json(const dyn_struct& s, size_t space_indent) noexcept;

extern const dyn_struct_const_array_iterator
begin(const dyn_struct_const_array_iterator_tag& d_struct) noexcept;
extern dyn_struct_array_iterator
begin(const dyn_struct_array_iterator_tag& d_struct) noexcept;
extern const dyn_struct_const_array_iterator
end(const dyn_struct_const_array_iterator_tag& d_struct) noexcept;
extern dyn_struct_array_iterator
end(const dyn_struct_array_iterator_tag& d_struct) noexcept;

extern const dyn_struct_structure_iterator
cbegin(const dyn_struct_structure_iterator_tag& d_struct) noexcept;
extern dyn_struct_structure_iterator
begin(const dyn_struct_structure_iterator_tag& d_struct) noexcept;
extern const dyn_struct_structure_iterator
cend(const dyn_struct_structure_iterator_tag& d_struct) noexcept;
extern dyn_struct_structure_iterator
end(const dyn_struct_structure_iterator_tag& d_struct) noexcept;

extern dyn_struct_array_iterator_tag iterate_array(dyn_struct& d_struct) noexcept;
extern dyn_struct_const_array_iterator_tag iterate_array(const dyn_struct& d_struct) noexcept;
extern dyn_struct_structure_iterator_tag iterate_structure(dyn_struct& d_struct) noexcept;

extern dyn_struct dyn_struct_array(size_t n = 0) noexcept;
extern size_t size(const dyn_struct&) noexcept;

constexpr extern bool holds_object(const dyn_struct& d_struct) noexcept;
constexpr extern bool holds_array(const dyn_struct& d_struct) noexcept;

constexpr extern bool holds_primitive(const dyn_struct& d_struct) noexcept;
constexpr extern bool holds_integer(const dyn_struct& d_struct) noexcept;
constexpr extern bool holds_real(const dyn_struct& d_struct) noexcept;
constexpr extern bool holds_bool(const dyn_struct& d_struct) noexcept;
constexpr extern bool holds_null(const dyn_struct& d_struct) noexcept;
constexpr extern bool holds_string(const dyn_struct& d_struct) noexcept;
constexpr extern bool holds_number(const dyn_struct& d_struct) noexcept;

namespace dyn_struct_error {
	constexpr auto NOT_AN_OBJECT = 1;
};

extern std::optional<dyn_struct>
load_from_json_file(const std::filesystem::path& file) noexcept;
extern size_t
save_to_json_file(const dyn_struct& to_save, const std::filesystem::path& file) noexcept;

template<typename T> void
from_dyn_struct(const dyn_struct& str, std::vector<T>& v) noexcept {
	for (const auto& x : iterate_array(str)) v.push_back((T)x);
}
template<typename T> void
to_dyn_struct(dyn_struct& str, const std::vector<T>& v) noexcept {
	str = dyn_struct::array_t{};
	for (const auto& x : v) str.push_back(x);
}
template<typename V> void
from_dyn_struct(const dyn_struct& to_save, std::unordered_map<std::string, V>& m) noexcept {
	for (auto& [k, v] : m) {
		str[k] = v;
	}
}
template<typename V> void
to_dyn_struct(dyn_struct& to_save, const std::unordered_map<std::string, V>& m) noexcept {
	for (auto& [k, v] : m) {
		str[k] = v;
	}
}
#endif