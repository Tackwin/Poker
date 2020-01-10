#pragma once

#include "Genome.hpp"

struct Population {
	size_t population_size;

	std::vector<Genome> genomes;
	std::vector<std::vector<size_t>> species;
	std::vector<Genome> specie_representatives;

	float specie_treshold = 2;

	float mutation_add_node = 0.005f;
	float mutation_del_node = 0.0001f;
	float mutation_add_connection = 0.05f;
	float mutation_del_connection = 0.001f;
	float mutation_weight = 0.1f;
	float mutation_weight_step = 0.05f;
	float mutation_activation = 0.1f;

	float speciation_size_inverse_power = 1.f;
	float age_inverse_power = 0.f;

	float to_kill = 0.5f;

	void selection() noexcept;
	void reproduction() noexcept;

	static Population generate(size_t pop_size, size_t n_inputs, size_t n_outputs) noexcept;

private:
	void generate_species() noexcept;
};