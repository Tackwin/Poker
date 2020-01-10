#pragma once

#include "IA/Population.hpp"
#include "IA/Genome.hpp"

#include <string>
#include <functional>
#include <array>
#include <thread>
#include <mutex>

struct ImGui_State;
struct Exp {
	std::string name = "Experiment";

	bool population_created = false;
	bool running = false;

	size_t generation_number = 0;
	int population_size = 0;
	Population pop;

	std::thread experiment_thread;
	std::mutex mutex;
	std::mutex epoch_mutex;
	std::condition_variable wait;

	std::vector<Genome> bests;
	std::vector<float> averages;

	std::vector<float> species_size;
	std::array<float, 100> cumulative_fitness;

	virtual void epoch() noexcept = 0;
	virtual void launch(size_t pop_size) noexcept;

	virtual void render(ImGui_State& imgui_state) noexcept;

	void render_stats(ImGui_State& imgui_state) noexcept;
	void render_params(ImGui_State& imgui_state) noexcept;
};

struct Xor_Exp : Exp {
	Xor_Exp() noexcept;

	std::array<float, 4> best_results;

	virtual void epoch() noexcept override;
	virtual void render(ImGui_State& state) noexcept override;
};

struct F_Exp : Exp {
	std::function<float(float)> f;

	F_Exp() noexcept;
	virtual void epoch() noexcept override;
};