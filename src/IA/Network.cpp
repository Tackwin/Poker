#include "Network.hpp"

#include <algorithm>
#include <cmath>

#include "macros.hpp"

float Network::Node::apply(float x, Network::Node::Activation act) noexcept {
	//return x > 0 ? 1 : 0;
	return 1 / (1 + std::expf(-x));
	switch(act) {
		case Node::Activation::Linear:
			return x;
		case Node::Activation::Relu:
			return x > 0 ? x : 0;
		case Node::Activation::Sig:
			return 1 / (1 + std::expf(-x));
		case Node::Activation::Sign:
			return x > 0 ? 1 : 0;
		default: return x;//return x + 1 > 0 ? x : -1;
	}
}

Network Network::generate(Genome genome) noexcept {
	Network net;

	struct Temp {
		Node n;
		size_t i;
	};
	std::vector<Temp> temps;

	for (auto& x : genome.node_genes) {
		Node node;
		node.kind = x.kind == NodeGene::Kind::LSTM ? Node::Kind::LSTM : Node::Kind::Simple;

		switch (x.func) {
		case NodeGene::Activation::Relu:
			node.func = Node::Activation::Relu;
			break;
		case NodeGene::Activation::Linear:
			node.func = Node::Activation::Linear;
			break;
		case NodeGene::Activation::Sig:
			node.func = Node::Activation::Sig;
			break;
		case NodeGene::Activation::Sign:
			node.func = Node::Activation::Sign;
			break;
		default:
			node.func = Node::Activation::Relu;
			break;
		}

		temps.push_back({ node, x.id });

		if (x.kind == NodeGene::Kind::Input) net.n_inputs++;
		if (x.kind == NodeGene::Kind::Output) net.n_outputs++;
	}

	std::sort(BEG_END(temps), [](const auto& a, const auto& b) { return a.i < b.i; });
	net.nodes.reserve(temps.size());
	net.links.resize(temps.size());

	for (auto& x : temps) net.nodes.push_back(x.n);

	for (auto& x : genome.connection_genes) {
		if (x.enabled)
			net.links[x.out].push_back({ (int)x.in, x.w });
	}

	return net;
}

std::vector<float> Network::compute(const std::vector<float>& inputs) noexcept {	
	for (size_t i = 0; i < n_inputs; ++i) {
		auto& x = nodes[i];
		x.activation = inputs[i];
		x.activesum = inputs[i];
		x.activated = true;
	}

	bool flag = false;
	size_t i = 0;
	do {
		flag = false;


		for (size_t i = n_inputs; i < nodes.size(); ++i) {
			auto& x = nodes[i];

			x.activesum = 0;
			x.activated = true;
			for (auto& l : links[i]) if (nodes[l.in].activated) {
				x.activesum += nodes[l.in].activation * l.w;
			} else {
				x.activated = false;
			}
		}

		for (size_t i = n_inputs; i < nodes.size(); ++i) {
			auto& x = nodes[i];
			if (!x.activated) {
				flag = true;
				continue;
			}

			x.activation = Node::apply(x.activesum, x.func);
		}

	} while(flag && i++ < 20);

	//bool at_least_one = false;
	//while (!at_least_one) {
	//	at_least_one = true;

	//	for (size_t i = n_inputs; i < nodes.size(); i++) {
	//		auto& x = nodes[i];
	//		if (x.fully_fed) continue;

	//		x.fully_fed = true;
	//		for (auto& l : links[i]) {
	//			if (l.in < 0) continue;
	//			if (!nodes[l.in].fully_fed) continue;
	//			x.fully_fed = false;
	//			at_least_one = false;

	//			x.activation += nodes[l.in].activation * l.w;
	//			l.in *= -1;
	//			l.in -= 1;
	//		}

	//		if (x.fully_fed) x.activation = Node::apply(x.activation, x.func);
	//	}
	//}

	//for (auto& x : links) for (auto& y : x) y.in = -(1 + y.in);

	std::vector<float> outputs;
	outputs.reserve(n_outputs);

	for (size_t i = n_inputs; i < n_inputs + n_outputs; ++i) outputs.push_back(nodes[i].activation);
	for (auto& x : nodes) { x.activated = false; x.activation = 0; x.activesum = 0; }

	return outputs;
}
