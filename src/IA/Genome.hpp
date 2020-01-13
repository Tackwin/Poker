#pragma once

#include <vector>
#include <set>
#include <string>

struct ConnectionGene {
	static inline auto Innov_N = 0;

	size_t in;
	size_t out;
	size_t innov;
	float w;
	bool enabled;
};

struct NodeGene {
	enum class Activation {
		Relu = 0,
		Linear,
		Sig,
		Sign,
		Count
	} func;
	enum class Kind {
		Input = 0,
		Hidden,
		Output,
		LSTM,
		Count
	} kind;
	size_t id;
};

struct Genome {
	std::vector<ConnectionGene> connection_genes;
	std::vector<NodeGene> node_genes;

	size_t n_inputs = 0;
	size_t n_outputs = 0;

	float fitness = 0;
	float adjusted_fitness = 0;

	float c1 = 1;
	float c2 = 1;
	float c3 = 0.4;

	float mutation_weight_step = 0.1f;

	size_t age = 0;

	bool marked = false;

	void add_node_mutation() noexcept;
	void weight_mutation(size_t i) noexcept;
	void del_node_mutation(size_t i) noexcept;
	void add_connection_mutation() noexcept;
	void activation_func_mutation(size_t i) noexcept;
	void remove_connection_mutation(size_t i) noexcept;

	std::string to_string() noexcept;

	static Genome generate(size_t n_inputs, size_t n_outputs) noexcept;
	static float speciation_coeff(const Genome& a, const Genome& b) noexcept;
	static Genome crossover(const Genome& parent1, const Genome& parent2) noexcept;
};
