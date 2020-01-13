#include "Genome.hpp"
#include "Random/Random.hpp"

#include <algorithm>

void Genome::add_connection_mutation() noexcept {
	size_t in = random(node_genes.size());
	size_t out = std::max(n_inputs, in) + random(node_genes.size() - std::max(n_inputs, in));

	auto& n_in = node_genes[in];
	auto& n_out = node_genes[out];

	if (in == out) return;
	for (auto& x : connection_genes)
		if ((x.in == in && x.out == out) || (x.in == out && x.out == in)) return;

	bool reversed =
		(n_in.kind == NodeGene::Kind::Hidden && n_out.kind == NodeGene::Kind::Input) ||
		(n_in.kind == NodeGene::Kind::Output && n_out.kind == NodeGene::Kind::Hidden) ||
		(n_in.kind == NodeGene::Kind::Output && n_out.kind == NodeGene::Kind::Input);

	ConnectionGene new_gene;
	new_gene.in = reversed ? out : in;
	new_gene.out = reversed ? in : out;
	new_gene.innov = ConnectionGene::Innov_N++;
	new_gene.w = randomf() * 2 - 1;
	new_gene.enabled = true;

	connection_genes.push_back(new_gene);
}

void Genome::add_node_mutation() noexcept {
	auto& connection = connection_genes[random(connection_genes.size())];
	connection.enabled = false;

	NodeGene new_node;
	new_node.id = node_genes.size();
	new_node.kind = NodeGene::Kind::Hidden;
	new_node.func = NodeGene::Activation::Relu;

	ConnectionGene first;
	first.in = connection.in;
	first.out = new_node.id;
	first.w = 1;
	first.enabled = true;
	first.innov = ConnectionGene::Innov_N++;

	ConnectionGene second;
	second.in = new_node.id;
	second.out = connection.out;
	second.w = connection.w;
	second.enabled = true;
	second.innov = ConnectionGene::Innov_N++;

	node_genes.push_back(new_node);
	connection_genes.push_back(first);
	connection_genes.push_back(second);
}

void Genome::del_node_mutation(size_t i) noexcept {
	//for (auto& x : connection_genes) {
	//	if (x.in == i || x.out == i) x.enabled = false;
	//}
}

void Genome::weight_mutation(size_t i) noexcept {
	connection_genes[i].w += randomnorm(0, mutation_weight_step);
}

void Genome::activation_func_mutation(size_t i) noexcept {
	auto x = (NodeGene::Activation)random((uint32_t)NodeGene::Activation::Count);
	node_genes[i].func = x;
}

void Genome::remove_connection_mutation(size_t i) noexcept {
	connection_genes[i].enabled = false;
}

Genome Genome::crossover(const Genome& parent1, const Genome& parent2) noexcept {
	Genome offspring;
	offspring.n_inputs = parent1.n_inputs;
	offspring.n_outputs = parent1.n_outputs;
	offspring.connection_genes.reserve(parent1.connection_genes.size());
	offspring.node_genes = parent1.node_genes;

	for (size_t i = 0; i < parent1.connection_genes.size(); ++i) {
		auto x = parent1.connection_genes[i];

		bool matching = false;
		const ConnectionGene* other = nullptr;
		for (size_t j = 0; j < parent2.connection_genes.size(); ++j) {
			auto& y = parent2.connection_genes[j];

			if (x.innov < y.innov) break;
			if (x.innov == y.innov) {
				matching = true;
				other = &y;
				x.w = (x.w + y.w) / 2;
				break;
			}
		}

		if (matching) offspring.connection_genes.push_back(randomf() > .5 ? x : *other);
		else          offspring.connection_genes.push_back(x);
	}


	return offspring;
}

float Genome::speciation_coeff(const Genome& a, const Genome& b) noexcept {
	size_t N = std::max(a.connection_genes.size(), b.connection_genes.size());
	size_t disjoints = 0;
	size_t excess = 0;
	size_t avg_n = 0;
	float avg_w = 0;

	size_t i = 0;
	size_t j = 0;
	for(; i < a.connection_genes.size() && j < b.connection_genes.size();) {
		auto& x = a.connection_genes[i];
		auto& y = b.connection_genes[j];

		if (x.innov == y.innov) { // Matching
			avg_w += std::abs(x.w - y.w);
			avg_n ++;

			i++;
			j++;
		}

		if (x.innov > y.innov) {
			disjoints ++;

			j++;
		}

		if (y.innov > x.innov) {
			disjoints ++;

			i++;
		}
	}

	excess += a.connection_genes.size() - i;
	excess += b.connection_genes.size() - j;

	float mult =
		1.f * std::max(a.node_genes.size(), b.node_genes.size()) /
		1.f * std::min(a.node_genes.size(), b.node_genes.size());

	return mult * (a.c1 * excess + a.c2 * disjoints) / N + a.c3 * avg_w / avg_n;
}

Genome Genome::generate(size_t n_inputs, size_t n_outputs) noexcept {
	Genome g;
//
	g.n_inputs = n_inputs;
	g.n_outputs = n_outputs;
//
	for (size_t i = 0; i < n_inputs; ++i) {
		NodeGene node_gene;
		node_gene.kind = NodeGene::Kind::Input;
		node_gene.func = NodeGene::Activation::Linear;
		node_gene.id = i;
		g.node_genes.push_back(node_gene);
	}
//
	for (size_t i = 0; i < n_outputs; ++i) {
		NodeGene node_gene;
		node_gene.kind = NodeGene::Kind::Output;
		node_gene.func = NodeGene::Activation::Linear;
		node_gene.id = i + n_inputs;
		g.node_genes.push_back(node_gene);
	}
//
	for (size_t i = 0; i < n_inputs; ++i) {
		for (size_t j = 0; j < n_outputs; ++j) {
			ConnectionGene connec;
			connec.innov = ConnectionGene::Innov_N++;
			connec.w = randomf() * 2 - 1;
			connec.enabled = true;
			connec.in = i;
			connec.out = j + n_inputs;
			g.connection_genes.push_back(connec);
		}
	}
	return g;

	static Genome genome;
	static bool flag = true;
	if (flag) {
		NodeGene node;
		node.kind = NodeGene::Kind::Input;

		node.id = 0;
		genome.node_genes.push_back(node);
		node.id = 1;
		genome.node_genes.push_back(node);
		node.id = 2;
		genome.node_genes.push_back(node);

		node.kind = NodeGene::Kind::Output;
		node.id = 3;
		genome.node_genes.push_back(node);

		node.kind = NodeGene::Kind::Hidden;
		node.id = 4;
		genome.node_genes.push_back(node);

		node.kind = NodeGene::Kind::Hidden;
		node.id = 5;
		genome.node_genes.push_back(node);

		ConnectionGene connec;
		//connec.enabled = true;
		//connec.in = 0;
		//connec.out = 3;
		//connec.w = -.5f;
		////connec.w = randomf() * 2 - 1;
		//connec.innov = ConnectionGene::Innov_N++;
		//genome.connection_genes.push_back(connec);
//
		//connec.in = 1;
		//connec.out = 3;
		//connec.w = 1;
		////connec.w = randomf() * 2 - 1;
		//connec.innov = ConnectionGene::Innov_N++;
		//genome.connection_genes.push_back(connec);
//
		//connec.in = 2;
		//connec.out = 3;
		//connec.w = 1;
		////connec.w = randomf() * 2 - 1;
		//connec.innov = ConnectionGene::Innov_N++;
		//genome.connection_genes.push_back(connec);

		connec.in = 4;
		connec.out = 3;
		connec.w = -2;
		//connec.w = randomf() * 2 - 1;
		connec.innov = ConnectionGene::Innov_N++;
		genome.connection_genes.push_back(connec);

		connec.in = 0;
		connec.out = 4;
		connec.w = -1.5f;
		//connec.w = randomf() * 2 - 1;
		connec.innov = ConnectionGene::Innov_N++;
		genome.connection_genes.push_back(connec);

		connec.in = 1;
		connec.out = 4;
		connec.w = 1;
		//connec.w = randomf() * 2 - 1;
		connec.innov = ConnectionGene::Innov_N++;
		genome.connection_genes.push_back(connec);

		connec.in = 2;
		connec.out = 4;
		connec.w = 1;
		//connec.w = randomf() * 2 - 1;
		connec.innov = ConnectionGene::Innov_N++;
		genome.connection_genes.push_back(connec);

		connec.in = 0;
		connec.out = 5;
		connec.w = -1.5f;
		//connec.w = randomf() * 2 - 1;
		connec.innov = ConnectionGene::Innov_N++;
		genome.connection_genes.push_back(connec);

		connec.in = 1;
		connec.out = 5;
		connec.w = 1;
		//connec.w = randomf() * 2 - 1;
		connec.innov = ConnectionGene::Innov_N++;
		genome.connection_genes.push_back(connec);

		connec.in = 2;
		connec.out = 5;
		connec.w = 1;
		//connec.w = randomf() * 2 - 1;
		connec.innov = ConnectionGene::Innov_N++;
		genome.connection_genes.push_back(connec);

		connec.in = 5;
		connec.out = 3;
		connec.w = 1;
		//connec.w = randomf() * 2 - 1;
		connec.innov = ConnectionGene::Innov_N++;
		genome.connection_genes.push_back(connec);
		
		connec.in = 0;
		connec.out = 3;
		connec.w = 1;
		//connec.w = randomf() * 2 - 1;
		connec.innov = ConnectionGene::Innov_N++;
		//genome.connection_genes.push_back(connec);

		genome.n_inputs = 3;
		genome.n_outputs = 1;
		flag = false;
	}

	return genome;
}

std::string Genome::to_string() noexcept {
	std::string result = "Age: " + std::to_string(age) + "\n[ :node: \n";
	for (auto& x : node_genes) {
		result += "{ ";
		switch (x.kind) {
		case NodeGene::Kind::Input:
			result += "Input";
			break;
		case NodeGene::Kind::Hidden:
			result += "Hidden";
			break;
		case NodeGene::Kind::Output:
			result += "Output";
			break;
		case NodeGene::Kind::LSTM:
			result += "LTSM";
			break;
		default:
			break;
		}
		result += ", ";
		
		switch (x.func) {
		case NodeGene::Activation::Relu:
			result += "Relu";
			break;
		case NodeGene::Activation::Linear:
			result += "Linear";
			break;
		case NodeGene::Activation::Sig:
			result += "Sigmoid";
			break;
		case NodeGene::Activation::Sign:
			result += "Sign";
			break;
		default:
			break;
		}

		result += ", " + std::to_string(x.id) = " }\n";
	}

	result.back() = ']';

	result += "\n[ :connection: \n";

	for (auto& x : connection_genes) {
		result +=
			"{ in: " + std::to_string(x.in) +
			" out: " + std::to_string(x.out) +
			" innov: " + std::to_string(x.innov) +
			" w: " + std::to_string(x.w) +
			" enabled: " + (x.enabled ? "true" : "false") + 
			"}\n";
	}

	result.back() = ']';

	return result;
}

