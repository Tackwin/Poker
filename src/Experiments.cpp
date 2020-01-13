#include "Experiments.hpp"

#include "IA/Genome.hpp"
#include "IA/Network.hpp"
#include "Profiler/Timer.hpp"

#include "macros.hpp"
#include <algorithm>

#include "imgui.h"
#include "Extensions/imgui_ext.h"

void render_genome(Genome& genome) noexcept {
	imnodes::BeginNodeEditor();

	for (auto& x : genome.node_genes) {
		std::string label = "Node";
		switch(x.func) {
			case NodeGene::Activation::Relu:
				label = "Relu";
				break;
			case NodeGene::Activation::Linear:
				label = "Line";
				break;
			case NodeGene::Activation::Sig:
				label = "Sigm";
				break;
			case NodeGene::Activation::Sign:
				label = "Sign";
				break;
			default:
				break;
		}
		
		imnodes::BeginNode(x.id * 3 + 0);
		defer { 
			imnodes::EndNode();
		};

		ImGui::Dummy({ImGui::GetWindowWidth() * 0.075f, 0});
		imnodes::BeginOutputAttribute(x.id * 3 + 1);
		imnodes::EndAttribute();
		ImGui::SameLine();
		imnodes::BeginInputAttribute(x.id * 3 + 2);
		imnodes::EndAttribute();
		ImGui::Text("Id: %zu\n", x.id);
		ImGui::Text("f: %s\n", label.c_str());
	}

	for (auto& x : genome.connection_genes) if (x.enabled) {
		imnodes::Link(x.innov, x.in * 3 + 1, x.out * 3 + 2);
	}

	imnodes::EndNodeEditor();
	
	int link_hovered = -1;
	if (imnodes::IsLinkHovered(&link_hovered)) {
		for (auto& x : genome.connection_genes) if(x.enabled && x.innov == link_hovered) {
			ImGui::BeginTooltip();
			ImGui::Text("%f\n", x.w);
			ImGui::EndTooltip();
		}
	}
}

void Exp::launch() noexcept {
	pop = Population::generate(pop.population_size, 3, 1);
	generation_number = 0;
	bests.clear();
	averages.clear();
	if (!population_created) {
		experiment_thread = std::thread([&] {
			std::mutex wait_mutex;
			while(true) {
				std::unique_lock<std::mutex> lk(wait_mutex);
				if (!running) wait.wait(lk, [&] { return running; });
				epoch();
			}
		});
		experiment_thread.detach();
	}

	population_created = true;
}

void Exp::render(ImGui_State& state) noexcept {
	ImGui::Begin(name.c_str());
	defer { ImGui::End(); };
	render_stats(state);
}

void Exp::render_stats(ImGui_State& imgui_state) noexcept {
	if (ImGui::CollapsingHeader("Params")) {
		ImGui::SliderFloat("Specie treshold", &pop.specie_treshold, 0, 10);
	}
	int x = pop.population_size;
	ImGui::SliderInt("Size", &x, 0, 100000);
	pop.population_size = x;
	if (ImGui::Button("Launch")) {
		launch();
	}
	if (!population_created) {
		ImGui::Text("You must first create a population.");
		return;
	}
	ImGui::SameLine();
	if (ImGui::Button("Step") && !running) {
		std::thread([&] { epoch(); }).detach();
	}
	ImGui::SameLine();
	if (running) {
		if (ImGui::Button("Pause")) {
			running = false;
			wait.notify_all();
		}
	} else if (ImGui::Button(" Run ")){
		running = true;
		wait.notify_all();
	}

	ImGui::Text("Generation: %zu", generation_number);

	ImGui::PlotLines("Best", [](void* data, int idx) {
		return ((Genome*)data)[idx].fitness;
	}, bests.data(), bests.size());

	ImGui::PlotLines("Averages", averages.data(), averages.size());
	ImGui::PlotHistogram("Cumulative", cumulative_fitness.data(), cumulative_fitness.size());
	ImGui::PlotHistogram(
		"Species size",
		species_size.data(),
		species_size.size(),
		0,
		0,
		0,
		1.f * pop.population_size
	);

	ImGui::Text("Species: %zu", pop.species.size());

	if (ImGui::CollapsingHeader("Genome") && !bests.empty()) {
		ImGui::BeginChild("Genomes");
		defer { ImGui::EndChild(); };
		ImGui::TextWrapped("%s\n", bests.back().to_string().c_str());
	}
}

void Exp::render_params(ImGui_State& imgui_state) noexcept {
	ImGui::SliderFloat("Specie Treshold", &pop.specie_treshold, 0, 10);
	ImGui::SliderFloat("To kill", &pop.to_kill, 0, 1);
	ImGui::SliderFloat("Add node", &pop.mutation_add_node, 0, 1);
	ImGui::SliderFloat("Del node", &pop.mutation_del_node, 0, 1);
	ImGui::SliderFloat("Add Connection", &pop.mutation_add_connection, 0, 1);
	ImGui::SliderFloat("Del Connection", &pop.mutation_del_connection, 0, 1);
	ImGui::SliderFloat("Weight", &pop.mutation_weight, 0, 1);
	ImGui::SliderFloat("Weight step", &pop.mutation_weight_step, 0, 1);
	ImGui::SliderFloat("Activation", &pop.mutation_activation, 0, 1);
	ImGui::SliderFloat("Speciation influence", &pop.speciation_size_inverse_power, 0, 2, "%.3f", 2);
	ImGui::SliderFloat("Age influence", &pop.age_influence, 0, 2, "%.3f", 2);
}

Xor_Exp::Xor_Exp() noexcept { name = "Xor"; }

void Xor_Exp::epoch() noexcept {
	std::unique_lock lock(epoch_mutex);
	Genome* best = nullptr;
	float avg = 0;
	static std::vector<float> fitnesses;
	fitnesses.reserve(pop.population_size);
	fitnesses.clear();

	for (auto& x : pop.genomes) {
		x.fitness = 0;
		auto net = Network::generate(x);

		float a = 0;
		a = 0 - net.compute({1, 0, 0}).front();
		x.fitness += std::abs(a);
		a = 1 - net.compute({1, 1, 0}).front();
		x.fitness += std::abs(a);
		a = 1 - net.compute({1, 0, 1}).front();
		x.fitness += std::abs(a);
		a = 0 - net.compute({1, 1, 1}).front();
		x.fitness += std::abs(a);
		x.fitness = 4 - x.fitness;

		avg += x.fitness;
		if (!best) best = &x;
		if (x.fitness > best->fitness) best = &x;

		fitnesses.push_back(x.fitness);
	}

	avg /= pop.genomes.size();

	{
		auto net = Network::generate(*best);
		best_results[0] = net.compute({1, 0, 0}).front();
		best_results[1] = net.compute({1, 1, 0}).front();
		best_results[2] = net.compute({1, 0, 1}).front();
		best_results[3] = net.compute({1, 1, 1}).front();
	}

	mutex.lock();
	bests.push_back(*best);
	averages.push_back(avg);
	species_size.clear();
	specie_bests.push_back({});
	for (auto& x : pop.species) {
		size_t best = x.front();
		for (auto& g : x) if (pop.genomes[best].fitness < pop.genomes[g].fitness) best = g;
		specie_bests.back().push_back(pop.genomes[best]);
	}
	for (auto& x : pop.species) species_size.push_back(1.f * x.size());
	mutex.unlock();

	pop.selection();
	pop.reproduction();
	pop.speciate();

	std::sort(BEG_END(fitnesses), [](auto a, auto b) { return a > b; });
	for (size_t i = 0; i < 100; ++i) {
		cumulative_fitness[i] = fitnesses[(size_t)(i * .01f * fitnesses.size())];
	}

	generation_number++;
}

void Xor_Exp::render(ImGui_State& state) noexcept {
	std::scoped_lock lock(mutex);
	ImGui::Begin(name.c_str());
	defer { ImGui::End(); };

	ImGui::BeginChild("Stats", {ImGui::GetWindowWidth() * 0.4f, 300});
	render_stats(state);
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Params", {ImGui::GetWindowWidth() * 0.4f, 300});
	render_params(state);
	ImGui::EndChild();

	ImGui::Separator();

	ImGui::Text("{0, 0} = %f", best_results[0]);
	ImGui::Text("{1, 0} = %f", best_results[1]);
	ImGui::Text("{0, 1} = %f", best_results[2]);
	ImGui::Text("{1, 1} = %f", best_results[3]);

	ImGui::Separator();

	int max_slider = specie_bests.empty() ? 0 : (int)(specie_bests.back().size()) - 1;
	max_slider = max_slider > 0 ? max_slider : 0;
	specie_selector = std::clamp(specie_selector, 0, max_slider);
	ImGui::SliderInt("X", &specie_selector, 0, max_slider);
	ImGui::Checkbox("By Specie", &view_by_species);

	if (view_by_species && specie_bests.size() > 1) {
		auto& x = specie_bests.back()[specie_selector];
		//ImGui::SameLine();
		//ImGui::Checkbox("Mark", &x.marked)
		render_genome(x);
	}
	else if (!bests.empty()) {
		render_genome(bests.back());
	}
}

F_Exp::F_Exp() noexcept { name = "F"; f = [](float x) { return x * x; }; }

void F_Exp::epoch() noexcept {
	Genome* best = nullptr;
	float avg = 0;
	for (auto& x : pop.genomes) {
		x.fitness = 0;
		auto net = Network::generate(x);
		for (float i = 0; i < 1; i += 1 / 10.f) {
			auto a = net.compute({1, i}).front() - f(i);
			x.fitness += std::abs(a);
		}

		avg += x.fitness;

		x.fitness = 1 / (x.fitness + 1);

		if (!best) best = &x;
		if (x.fitness > best->fitness) best = &x;
	}

	avg /= pop.genomes.size();

	mutex.lock();
	bests.push_back(*best);
	averages.push_back(avg);
	mutex.unlock();

	printf("Gen: %zu => %f\n", generation_number, 1 / best->fitness - 1);
	printf("Species: %zu\n", pop.species.size());
	{
		auto net = Network::generate(*best);
		for (float i = 0; i < 1; i += 1 / 10.f) {
			printf("f(%f) = %f (%f)\n", i, net.compute({1, i}).front(), f(i));
		}
	}
	printf("%s\n", best->to_string().c_str());

	pop.selection();
	pop.reproduction();
	generation_number++;
}