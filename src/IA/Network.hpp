#pragma once

#include "Genome.hpp"

struct Network {
	struct Link {
		int in;
		float w;
	};
	
	struct Node {
		float activation = 0;
		float activesum = 0;

		enum class Activation : std::uint8_t {
			Sig = 0,
			Linear,
			Tanh,
			Relu,
			Sign,
			Cos,
			Sin,
			Mult,
			Add,
			Count
		} func;

		enum class Kind : std::uint8_t {
			Simple,
			LSTM,
			Count
		} kind;

		bool activated = false;
		static float apply(float x, Node::Activation act) noexcept;
	};

	std::vector<Node> nodes;
	std::vector<std::vector<Link>> links;

	size_t n_inputs = 0;
	size_t n_outputs = 0;

	std::vector<float> compute(const std::vector<float>& inputs) noexcept;

	static Network generate(Genome genome) noexcept;
};