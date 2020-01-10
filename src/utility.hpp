#pragma once
#include <SFML/Graphics.hpp>
#include <optional>

#include <Windows.h>
struct Window {
	HWND handle;
};

extern Window get_desktop_window() noexcept;
extern std::optional<Window> find_winamax_window() noexcept;
extern std::optional<sf::Image> take_screenshot(Window window) noexcept;


