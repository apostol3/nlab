#pragma once

#include "neuron.h"
#include <string>
#include <cstdint>
namespace nlab {
typedef std::vector< double > net_task;

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

	tweann(const tweann& n)
	{
		//TODO: change this to normal copying constructor
		*this = n;
	}

	tweann& operator=(const tweann& n)
	{
		nr = n.nr;
		lr = n.lr;
		nr.link_rep_ = &lr;
		lr.neuron_rep_ = &nr;
		fitness = n.fitness;
		id = generate_id();
		return *this;
	}

private:
	static std::uint64_t generate_id();
};
};