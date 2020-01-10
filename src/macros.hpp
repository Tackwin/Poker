#pragma once

namespace details {
	template<typename Callable>
	struct Defer {
		~Defer() noexcept { todo(); }
		Defer(Callable todo) noexcept : todo(todo) {};
	private:
		Callable todo;
	};
};

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)
#define defer details::Defer CONCAT(defer_, __COUNTER__) = [&]

#define BEG(x) std::begin(x)
#define END(x) std::end(x)
#define BEG_END(x) BEG(x), END(x)
#define V2F(x) Vector2f{(x), (x)}

#define IF(cond, foo) IF_IMPL(cond, foo)
#define IF_NOT(cond, foo) IF_NOT_IMPL(cond, foo)
#define IF_IMPL(cond, foo) IF_##cond (foo)
#define IF_NOT_IMPL(cond, foo) IF_NOT_##cond (foo)
#define IF_0(foo)
#define IF_1(foo) foo
#define IF_NOT_1(foo)
#define IF_NOT_0(foo) foo
