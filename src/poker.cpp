#include "poker.hpp"
#include <algorithm>
#include <assert.h>
#include <string>
#include <stdio.h>

#include <time.h>
#include <random>

#include <thread>

#include "macros.hpp"

#include "Random/Random.hpp"

std::vector<size_t> pick_winners(
	const std::array<Player, 3>& players, std::array<Card, 5> board
) noexcept;

Game::Game() noexcept {
	for (size_t i = 0; i < agents.size(); ++i) {
		agents[i].me = &players[i];
		players[i].agent = &agents[i];
		players[i].stack = 500;
	}
}

bool Game::over() noexcept {
	size_t i = 0;
	
	for (auto& x : players) {
		if (x.stack > 0) i++;
		if (i > 1) return false;
	}

	return true;
}

void Game::print_game() noexcept {
	printf("[0:%s] %zu\n", players[0].name.c_str(), players[0].stack);
	printf("[1:%s] %zu\n", players[1].name.c_str(), players[1].stack);
	printf("[2:%s] %zu\n", players[2].name.c_str(), players[2].stack);
}


void Game::play_game() noexcept {
	if (verbose) print_game();
	
	while (!over()) {
		//using namespace std::literals::chrono_literals;
		//std::this_thread::sleep_for(1s);
		play_new_hand();

		if (verbose) print_game();
	}
}

void Game::play_new_hand() noexcept {
	current_hand = {};

	for (auto& x : players) {
		x.bet = 0;
		x.current_bet = 0;
		x.folded = false;

		for (size_t i = 0; i < x.hand.size(); ++i) {
			auto select = rand() % current_hand.draw.size();
			x.hand[i] = current_hand.draw[select];
			current_hand.draw.erase(BEG(current_hand.draw) + select);
		}
	}

	big_bling_idx %= players.size();

	auto& big_blind_player = players[big_bling_idx];
	auto& small_blind_player = players[((players.size() + big_bling_idx) - 1) % players.size()];

	current_hand.pot += std::min(big_blind_player.stack, big_blind);
	big_blind_player.stack -= std::min(big_blind_player.stack, big_blind);
	current_hand.pot += std::min(small_blind_player.stack, big_blind / 2);
	small_blind_player.stack -= std::min(small_blind_player.stack, big_blind / 2);
	
	running_bet = big_blind;

	auto round = [&] {
		for (size_t i = 0; i < players.size(); ++i) {
			auto idx = (i + big_bling_idx + 1) % players.size();
			if (players[idx].folded) continue;

			auto act = agents[idx].act(*this);
			apply(players[idx], act);
		}

		raised_turn = true;
		defer{ raised_turn = false; };
		
		if (running_bet <= big_blind) return;
		
		for (size_t i = 0; i < players.size(); ++i) {
			auto idx = (i + big_bling_idx + 1) % players.size();
			if (players[idx].folded) continue;

			auto act = agents[idx].act(*this);
			apply(players[idx], act);
		}

		running_bet = 0;
		for (auto& x : players) {
			x.current_bet = 0;
		}
	};

	for (auto& x : players) {
		if (x.stack == 0) {
			x.folded = true;
		}
	}

	round();

	for (size_t i = 0; i < current_hand.flop.size(); ++i) {
		auto select = rand() % current_hand.draw.size();
		current_hand.flop[i] = current_hand.draw[select];
		current_hand.draw.erase(BEG(current_hand.draw) + select);
	}

	round();

	auto select = rand() % current_hand.draw.size();
	current_hand.turn = current_hand.draw[select];
	current_hand.draw.erase(BEG(current_hand.draw) + select);

	round();

	select = rand() % current_hand.draw.size();
	current_hand.river = current_hand.draw[select];
	current_hand.draw.erase(BEG(current_hand.draw) + select);

	round();

	auto winners = pick_winners(players, {
		current_hand.flop[0],
		current_hand.flop[1],
		current_hand.flop[2],
		current_hand.turn,
		current_hand.river
	});

	for (auto& x : winners) {
		auto& p = players[x];
		p.stack += current_hand.pot / winners.size();
		if (verbose) printf("Joueur %zu (%s) a gagne.\n", x, players[x].name.c_str());
	}

	big_bling_idx++;

	passed_hands.push_back(current_hand);
}

void Game::apply(Player& player, Action action) noexcept {
	switch (action.kind) {
	case Action::Check: break;
	case Action::Fold: {
		player.folded = true;
		break;
	}
	case Action::Follow: {
		assert((player.stack + player.current_bet) >= running_bet);
		size_t dt = running_bet - player.current_bet;

		player.stack -= dt;
		current_hand.pot += dt;
		player.current_bet = running_bet;
		player.bet += dt;
		break;
	}
	case Action::Raise: {
		assert(player.stack >= action.value);
		player.stack -= action.value;

		current_hand.pot += action.value;
		
		running_bet += std::max(running_bet, action.value) - running_bet;

		player.current_bet += action.value;
		player.bet += action.value;
		break;
	}
	case Action::None: {
		assert(player.stack == 0 && !player.folded);
		break;
	}
	default: assert("Logic error.");
	}
}

Action Agent::act(const Game& game) noexcept {
	Action action;
	if (me->stack == 0) {
		action.kind = Action::None;
		return action;
	}

	if (game.raised_turn) {
		if (game.running_bet > me->stack) action.kind = Action::Fold;
		else action.kind = Action::Follow;
	}
	else {
		bool can_raise{ false };
		size_t to_raise = std::min(game.running_bet + 10u, me->stack);

		for (auto& x : game.players) {
			if (me == &x) continue;

			if (x.stack + x.current_bet > to_raise + game.running_bet) {
				can_raise = true;
				break;
			}
		}

		if (can_raise) {
			action.kind = Action::Raise;
			action.value = to_raise;
		}
		else {
			action.kind = Action::Follow;
		}
	}

	if (game.verbose) {
		printf("%s plays: %s\n", me->name.c_str(), action.stringify().c_str());
	}

	return action;
}

std::string Action::stringify() noexcept {
	switch (kind) {
		case Follow: return "Follow";
		case Raise: return "Raise " + std::to_string(value);
		case Check: return "Check";
		case Fold: return "Fold";
		default: return "-----";
	}
}


Deck::Deck() noexcept {
	reserve((size_t)Color::Size * (size_t)Value::Size);
	for (size_t i = 0; i < (size_t)Color::Size; ++i) {
		for (size_t j = 0; j < (size_t)Value::Size; ++j) {
			push_back({ (Color)i, (Value)j });
		}
	}

	seed = (size_t)time(nullptr);

	std::default_random_engine rng(seed);

	shuffle<Card>(data(), size());
	//std::shuffle(BEG_END(*this), rng);
}

std::vector<size_t> pick_winners(
	const std::array<Player, 3>& players, std::array<Card, 5> board
) noexcept {
	struct Combo {
		enum Kind {
			High = 0,
			Pair,
			Two_Pair,
			Three_Of_A_Kind,
			Straight,
			Flush,
			Full,
			Four_Of_A_Kind,
			Straight_Flush,
			Royal_Flush,
			Size
		} kind{ High };

		std::array<Card, 5> cards;
		size_t player_idx;

		bool operator==(const Combo& other) const noexcept {
			if (kind != other.kind) return false;
			for (size_t i = 0; i < cards.size(); ++i) {
				if (cards[i].value != other.cards[i].value) return false;
			}
			return true;
		}
		bool operator<(const Combo& other) const noexcept {
			if (kind < other.kind) return true;
			if (kind > other.kind) return false;
			if (cards < other.cards) return true;
			return false;
		}
		bool operator>(const Combo& other) const noexcept {
			if (kind > other.kind) return true;
			if (kind < other.kind) return false;
			if (cards > other.cards) return true;
			return false;
		}
	};

#define E END(hand)
#define X BEG_END(hand)
	auto get_dominant_color = [](const std::array<Card, 7>& hand) -> std::pair<Color, size_t> {
		std::array<size_t, (size_t)Color::Size> n{};
		Color dominant{ Color::Club };

		for (size_t i = 0; i < 4; ++i) ++n[i];
		for (size_t i = 0; i < n.size(); ++i) if (n[(size_t)dominant] < n[i]) dominant = (Color)i;

		return { dominant, n[(size_t)dominant] };
	};

	auto test_four_of_a_kind = [&](const std::array<Card, 7> & hand) -> bool {
		for (size_t i = 0; i < 4; ++i) {
			bool flag =
				std::find(X, Card{ Color::Club, hand[i].value }) != E &&
				std::find(X, Card{ Color::Diamond, hand[i].value }) != E &&
				std::find(X, Card{ Color::Heart, hand[i].value }) != E &&
				std::find(X, Card{ Color::Spade, hand[i].value }) != E;
			if (flag) return true;
		}
		return false;
	};

	auto test_three_of_a_kind = [&](const std::array<Card, 7> & hand) -> bool {
		for (size_t i = 0; i < 5; ++i) {
			size_t n =
				std::count(X, Card{ Color::Club, hand[i].value }) +
				std::count(X, Card{ Color::Diamond, hand[i].value }) +
				std::count(X, Card{ Color::Heart, hand[i].value }) +
				std::count(X, Card{ Color::Spade, hand[i].value });

			if (n >= 3) return true;
		}
		return false;
	};

	auto test_pair = [&](const std::array<Card, 7> & hand) -> bool {
		for (size_t i = 0; i < 6; ++i) {
			size_t n =
				std::count(X, Card{ Color::Club, hand[i].value }) +
				std::count(X, Card{ Color::Diamond, hand[i].value }) +
				std::count(X, Card{ Color::Heart, hand[i].value }) +
				std::count(X, Card{ Color::Spade, hand[i].value });

			if (n >= 2) return true;
		}
		return false;
	};

	auto test_two_pair = [&](const std::array<Card, 7> & hand) -> bool {
		size_t n_pair = 0;

		for (size_t i = 0; i < 6; ++i) {
			size_t n =
				std::count(X, Card{ Color::Club, hand[i].value }) +
				std::count(X, Card{ Color::Diamond, hand[i].value }) +
				std::count(X, Card{ Color::Heart, hand[i].value }) +
				std::count(X, Card{ Color::Spade, hand[i].value });

			if (n >= 2) n_pair++;

			// Every pair is counted twice so we need to go to three pairs to detect a double pair.
			if (n_pair > 2) return true;
		}
		return false;
	};

	auto test_royal_flush = [&](const std::array<Card, 7>& hand) -> bool {
		auto [dominant, n] = get_dominant_color(hand);
		if (n < 5) return false;

		return
			std::find(X, Card{ dominant, Value::As }) != E &&
			std::find(X, Card{ dominant, Value::King }) != E &&
			std::find(X, Card{ dominant, Value::Queen }) != E &&
			std::find(X, Card{ dominant, Value::Jacket }) != E &&
			std::find(X, Card{ dominant, Value::Ten }) != E
		;
	};

	auto test_straight = [&](const std::array<Card, 7>& hand) -> bool {
		for (size_t i = 0; i < 7; ++i) {
			auto any_color = [&](Value value) {
				return std::find_if(X, [&](auto x) {return x.value == value; }) != E;
			};

			size_t start_value = (size_t)hand[i].value;
			if (start_value == (size_t)Value::As) start_value = (size_t)(-1);
			if (start_value > (size_t)Value::Ten) continue;

			for (size_t j = 0; j < 4; ++j) {
				auto to_search = (Value)((start_value + j + 1) % (size_t)Value::Size);
				if (!any_color(to_search)) goto up_continue;
			}

			return true;
			up_continue:
			;
		}
		return false;
	};

	auto test_full = [&](const std::array<Card, 7> & hand) -> bool {
		size_t n_pair = 0;
		size_t n_brelan = 0;


		for (size_t i = 0; i < 6; ++i) {
			size_t n =
				std::count(X, Card{ Color::Club, hand[i].value }) +
				std::count(X, Card{ Color::Diamond, hand[i].value }) +
				std::count(X, Card{ Color::Heart, hand[i].value }) +
				std::count(X, Card{ Color::Spade, hand[i].value });

			if (n == 2) n_pair++;
			if (n == 3) n_brelan++;

			if (n_pair && n_brelan) return true;
		}

		return false;
	};

	auto test_flush = [&](const std::array<Card, 7> & hand) -> bool {
		return get_dominant_color(hand).second >= 5;
	};

	auto test_straight_flush = [&](const std::array<Card, 7>& hand) -> bool {
		return test_straight(hand) && test_flush(hand);
	};
#undef X
#undef E

	std::vector<Combo> combos;

	for (size_t i = 0; i < players.size(); ++i) {
		Combo combo;
		combo.player_idx = i;
		auto& p = players[i];

		if (p.folded) continue;

		defer{ combos.push_back(combo); };

		std::array<Card, 7> combined_hand = {
			p.hand[0],
			p.hand[1],
			board[0],
			board[1],
			board[2],
			board[3],
			board[4]
		};

		if (test_royal_flush(combined_hand)) {
			combo.kind = Combo::Kind::Royal_Flush;
		}
		else if (test_straight_flush(combined_hand)) {
			combo.kind = Combo::Kind::Straight_Flush;
		}
		else if (test_four_of_a_kind(combined_hand)) {
			combo.kind = Combo::Kind::Four_Of_A_Kind;
		}
		else if (test_four_of_a_kind(combined_hand)) {
			combo.kind = Combo::Kind::Four_Of_A_Kind;
		}
		else if (test_full(combined_hand)) {
			combo.kind = Combo::Kind::Full;
		}
		else if (test_flush(combined_hand)) {
			combo.kind = Combo::Kind::Flush;
		}
		else if (test_straight(combined_hand)) {
			combo.kind = Combo::Kind::Straight;
		}
		else if (test_three_of_a_kind(combined_hand)) {
			combo.kind = Combo::Kind::Three_Of_A_Kind;
		}
		else if (test_two_pair(combined_hand)) {
			combo.kind = Combo::Kind::Two_Pair;
		}
		else if (test_pair(combined_hand)) {
			combo.kind = Combo::Kind::Pair;
		}
		else {
			combo.kind = Combo::Kind::High;
		}

		std::sort(BEG_END(combined_hand), [](auto a, auto b) {
			if (a.value == Value::As && b.value != Value::As) return true;
			if (a.value != Value::As && b.value == Value::As) return false;
			return (size_t)a.value > (size_t)b.value;
		});

		for (size_t j = 0; j < 5; j++) combo.cards[j] = combined_hand[j];
	}

	std::sort(BEG_END(combos), [](auto& a, auto& b) { return a > b; });

	std::vector<size_t> winners;
	winners.push_back(combos.front().player_idx);
	for (size_t i = 1; i < combos.size(); ++i) {
		if (!(combos[i] == combos[i - 1])) return winners;
		winners.push_back(combos[i].player_idx);
	}
	return winners;
}

