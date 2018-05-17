#pragma once

#include "neuron.h"

#include <string>
#include <cstdint>

namespace nlab {

using net_task = std::vector< double >;

class tweann
{
public:
	int reset();
	net_task calc(const net_task& task);

	tweann() : tweann(3, 1) { }

	tweann(size_t in, size_t out);

	neuron_rep nr;
	link_rep lr;
	double fitness;
	std::uint64_t id;

	std::wstring note;
	std::wstring name;

	tweann(const tweann& n) : nr(n.nr), lr(n.lr), fitness(n.fitness), id(generate_id()),
		note(n.note), name(n.name)
	{
		nr.link_rep_ = &lr;
		lr.neuron_rep_ = &nr;
	}

	tweann& operator=(const tweann& n)
	{
		if (this == &n) {
			return *this;
		}

		nr = n.nr;
		lr = n.lr;
		nr.link_rep_ = &lr;
		lr.neuron_rep_ = &nr;
		fitness = n.fitness;
		id = generate_id();
		note = n.note;
		name = n.name;

		return *this;
	}

private:
	static std::uint64_t generate_id();
};

} // namespace nlab