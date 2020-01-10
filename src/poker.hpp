#pragma once
#include <array>
#include <vector>

enum class Color {
	Spade = 0,
	Heart,
	Diamond,
	Club,
	Size
};
enum class Value {
	Two = 0,
	Three,
	Four,
	Five,
	Six,
	Seven,
	Eight,
	Nine,
	Ten,
	Jacket,
	Queen,
	King,
	As,
	Size
};

struct Card {
	Color color;
	Value value;

	bool operator==(const Card& other) const noexcept {
		return color == other.color && value == other.value;
	}
	bool operator<(const Card& other) const noexcept {
		return value < other.value;
	}
	bool operator>(const Card& other) const noexcept {
		return value > other.value;
	}
};

struct Deck : public std::vector<Card> {
	size_t seed;

	Deck() noexcept;
};

struct Agent;
struct Player {
	Agent* agent{ nullptr };

	std::string name;

	std::array<Card, 2> hand{};
	size_t stack{ 0 };

	size_t bet{ 0 };
	size_t current_bet{ 0 };
	bool folded{ false };
};

struct Hand {
	Deck draw;

	std::array<Card, 3> flop;
	Card turn;
	Card river;

	size_t pot{ 0 };
	size_t big_blind{ 0 };
};

struct Action {
	size_t value;
	enum {
		Follow = 0,
		Raise,
		Check,
		None,
		Fold,
		Size
	} kind;

	std::string stringify() noexcept;
};
struct Game;
struct Agent {
	Player* me;

	Action act(const Game& game) noexcept;
};

struct Game {
	Game() noexcept;

	std::vector<Hand> passed_hands;
	Hand current_hand;

	std::array<Player, 3> players;
	std::array<Agent, 3> agents;

	size_t big_blind{ 10 };
	size_t big_bling_idx{ 0 };
	
	size_t running_bet{ 0 };

	bool raised_turn{ false };

	void play_game() noexcept;
	void play_new_hand() noexcept;
	void apply(Player& player, Action x) noexcept;

	bool over() noexcept;

	void print_game() noexcept;

	bool verbose{false};
};

