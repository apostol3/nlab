#include "tweann.h"

#include <chrono>
#include <random>

using namespace nlab;

std::uint64_t tweann::generate_id()
{
	static std::uint64_t seed = std::chrono::system_clock::now().time_since_epoch().count();
	static std::mt19937_64 gen(seed);
	static std::uniform_int_distribution< std::uint32_t > dist; //borland compiler fails with 
	//uint64_t distribution
	return (std::uint64_t(dist(gen)) << 32) | std::uint64_t(dist(gen));
}

/* */
tweann::tweann(size_t in, size_t out)
{
	int maxx = 392;
	int maxy = 491;
	int fx = 10;
	int fy = 10;
	int r = 20;

	nr.link_rep_ = &lr;
	lr.neuron_rep_ = &nr;
	for (size_t i = 0; i < in + out; i++)
	{
		neuron* n = nr.get(nr.insert(neuron())); // TODO: try optimize
		if (i < in)
		{
			n->c->type = input;
		}
		else
		{
			n->c->type = output;
		}

		n->c->e = 0;
		n->c->ea = 1;

		if (i < in)
		{
			int dx = (maxx - 2 * fx - 2 * r) / (static_cast< unsigned >(in) + 1);
			n->v->y = 0 + fy + r;
			n->v->x = 0 + fx + dx * static_cast< unsigned >(i + 1) + r;
			n->v->r = r;
		}
		else
		{
			int dx = (maxx - 2 * fx - 2 * r) / static_cast< unsigned >(out+1);
			n->v->y = maxy - fy - r;
			n->v->x = 0 + fx + dx * static_cast< unsigned >(i - in + 1) + r;
			n->v->r = r;
		}
	}

	fitness = 0;
	id = generate_id();
}

/* */
int tweann::reset()
{
	for (auto& i : nr.neurons)
	{
		i.c->e = 0;
	}

	for (auto& i : lr.links)
	{
		i.in_e = 0;
		i.out_e = 0;
	}

	return 0;
}

/* */
net_task tweann::calc(const net_task& task)
{
	std::vector< neuron_c * > n_in;
	std::vector< neuron_c * > n_out;

	n_in.reserve(task.size());
	size_t sz = nr.neurons.size();
	for (size_t i = 0; i < sz; i++)
	{
		neuron_c* nc = nr.neurons[i].c;
		if (nc->type == neuron_type::input)
		{
			n_in.push_back(nc);
			continue;
		}

		if (nc->type == neuron_type::output)
		{
			n_out.push_back(nc);
		}
	}

	if (n_in.size() != task.size())
	{
		throw;
	}

	if (n_out.empty())
	{
		throw;
	}

	sz = n_in.size();
	for (size_t i = 0; i < sz; i++)
	{
		n_in[i]->e += task[i];
	}

	sz = nr.neurons.size();
	for (size_t i = 0; i < sz; i++)
	{
		nr.neurons[i].c->calc();
	}

	sz = lr.links.size();
	for (size_t j = 0; j < sz; j++)
	{
		lr.links[j].calc();
	}

	net_task out;
	sz = n_out.size();
	out.reserve(sz);
	for (size_t i = 0; i < sz; i++)
	{
		out.push_back(n_out[i]->e);
		n_out[i]->e = 0;
	}

	return out;
}

