#include "Population.hpp"

#include "Random/Random.hpp"
#include "macros.hpp"

#include "Profiler/Timer.hpp"

#include <algorithm>

void Population::selection() noexcept {
	generate_species();

	float fitness_sum = 0;
	for (size_t i = 0; i < genomes.size(); ++i) {
		size_t n = 0;
		for (auto& specie : species) {
			if (std::find(BEG_END(specie), i) != END(specie)) continue;
			n = specie.size();
			break;
		}

		float divisor = std::max(n, (size_t)20);
		divisor = std::powf(divisor, speciation_size_inverse_power);
		divisor *= std::powf(genomes[i].age, age_inverse_power);

		genomes[i].adjusted_fitness = genomes[i].fitness / divisor;
		fitness_sum += genomes[i].adjusted_fitness;
	}

	float to_select = to_kill * to_kill;

	auto lamb = [&](auto a, auto b) {
		return genomes[a].adjusted_fitness > genomes[b].adjusted_fitness;
	};
	for (auto& specie : species) {
		std::sort(BEG_END(specie), lamb);
		specie.resize((size_t)(to_select * specie.size()));
	}

	auto pred = [&, i = 0](const Genome& x) mutable {
		bool flag = false;
		for (auto& specie : species) for (auto& j : specie) if (i == j) { flag = true; goto next; };
		next:
		i++;
		return flag;
	};
	genomes.erase(std::remove_if(BEG_END(genomes), pred), END(genomes));
	std::sort(
		BEG_END(genomes),
		[](const auto& a, const auto& b) { return a.adjusted_fitness > b.adjusted_fitness; }
	);
	genomes.resize((size_t)(to_select * genomes.size()));

	//for (size_t i = 0; i < to_select; ++i) {
	//	float prob = 0;
	//	size_t selec = 0;
//
	//	do {
	//		selec = random(genomes.size());
	//		prob = genomes[i].adjusted_fitness / fitness_sum;
	//	} while (randomf() > prob);
//
	//	selected.push_back(genomes[selec]);
	//	fitness_sum -= genomes[selec].adjusted_fitness;
	//	genomes[selec] = genomes.back();
	//	genomes.pop_back();
	//}

	//genomes.swap(selected);
}

void Population::reproduction() noexcept {
	size_t parent_size = genomes.size();
	size_t to_birth = population_size - genomes.size();

	float fitness_sum = 0;
	for (auto& x : genomes) fitness_sum += x.adjusted_fitness;

	for (size_t i = 0; i < to_birth; ++i) {
		auto r1 = randomf() * fitness_sum;
		auto r2 = randomf() * fitness_sum;

		size_t p1 = 0;
		size_t p2 = 0;

		for (size_t j = 0; j < parent_size; ++j) {
			if (r1 < 0 && r2 < 0) break;

			auto& x = genomes[j];
			r1 -= x.adjusted_fitness;
			r2 -= x.adjusted_fitness;

			if (r1 < 0) {
				p1 = j;
				continue;
			}

			if (r2 < 0) {
				p2 = j;
				continue;
			}
		}

		if (genomes[p1].adjusted_fitness > genomes[p2].adjusted_fitness) std::swap(p1, p2);

		auto it = Genome::crossover(genomes[p1], genomes[p2]);
		if (randomf() < mutation_add_node) it.add_node_mutation();
		if (randomf() < mutation_add_connection) it.add_connection_mutation();
		for (size_t i = 0; i < it.connection_genes.size(); ++i){
			if (randomf() < mutation_weight) it.weight_mutation(i);
			if (randomf() < mutation_del_connection) it.remove_connection_mutation(i);
		}
		for (size_t i = 0; i < it.node_genes.size(); ++i)
			if (randomf() < mutation_activation) it.activation_func_mutation(i);

		genomes.push_back(it);
	}

	for (auto& x : genomes) x.age++;
}

void Population::generate_species() noexcept {

	species.clear();
	species.resize(specie_representatives.size());

	for (size_t i = 0; i < genomes.size(); ++i) {
		auto& x = genomes[i];
		
		for (size_t j = 0; j < specie_representatives.size(); ++j) {
			if (Genome::speciation_coeff(x, specie_representatives[j]) < specie_treshold) {
				species[j].push_back(i);
				goto continue_outer;
			}
		}

		specie_representatives.push_back(x);
		species.push_back({i});

continue_outer:
		continue;
	}

	for (size_t i = species.size() - 1; i + 1 > 0 ; i--) {
		if (species[i].size() == 0) {
			species[i] = species.back();
			species.pop_back();
			specie_representatives[i] = specie_representatives.back();
			specie_representatives.pop_back();
		}
	}

}

Population Population::generate(size_t pop_size, size_t n_inputs, size_t n_outputs) noexcept {
	Population pop;
	pop.population_size = pop_size;
	Genome g = Genome::generate(n_inputs, n_outputs);
	for (size_t i = 0; i < pop_size; ++i) pop.genomes.push_back(g);
	return pop;
}

