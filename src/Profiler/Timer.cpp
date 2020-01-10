#include "Timer.hpp"

#include <chrono>

size_t nanoseconds() noexcept {
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

}
double microseconds() noexcept {
	return nanoseconds() / 1'000.0;
}
double milliseconds() noexcept {
	return microseconds() / 1'000.0;
}
double seconds() noexcept {
	return milliseconds() / 1'000.0;
}
