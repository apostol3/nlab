#include "neuron.h"
#include <cmath>

using namespace nlab;

/* */
double neuron_c::get_ei()
{
	double s = 0;
	size_t sz = in.size();
	for (size_t i = 0; i < sz; i++)
	{
		link* l = parent()->rep->link_rep_->get(in[i]); // TODO: cache
		if (l == nullptr)
		{
			continue;
		}

		double k = l->out_e;
		l->out_e = 0;
		s += k;
	}

	return s;
}

/* */
void neuron_c::set_eo(double eo)
{
	double s1 = 0;
	size_t sz = out.size();
	for (size_t i = 0; i < sz; i++)
	{
		link* l = parent()->rep->link_rep_->get(out[i]); // TODO: cache
		if (l == nullptr)
		{
			continue;
		}

		s1 += std::fabs(l->w);
	}

	double lamb = 0;
	if (s1 != 0) //TODO: V550 http://www.viva64.com/en/V550 An odd precise comparison: 
	//s1 != 0. It's probably better to use a comparison with defined precision: 
	//fabs(A - B) > Epsilon.
	{
		lamb = 1.f / s1;
	}

	sz = out.size();
	for (size_t i = 0; i < sz; i++)
	{
		link* l = parent()->rep->link_rep_->get(out[i]); // TODO: cache
		if (l == nullptr)
		{
			continue;
		}

		l->in_e += eo * l->w * lamb;
	}
}

/* */
void neuron_c::calc()
{
	switch (this->type)
	{
		case neuron_type::input:
			set_eo(e);
			e = 0;
			break;

		case neuron_type::output:
			e = get_ei();
			break;

		case neuron_type::blank:
			set_eo(e);
			e = get_ei();
			break;

		case neuron_type::activ:
			if (e < ea)
			{
				set_eo(0);
			}
			else
			{
				set_eo(e);
				e = 0;
			}

			e += get_ei();
			break;

		case neuron_type::limit:
			if (e < ea)
			{
				set_eo(0);
			}
			else
			{
				set_eo(ea);
				e -= ea;
			}

			e += get_ei();
			break;

		case neuron_type::binary:
			if (e < ea)
			{
				set_eo(0);
				e = 0;
			}
			else
			{
				set_eo(ea);
				e = 0;
			}

			e = get_ei();
			break;

		case neuron_type::gen:
			if (e < ea)
			{
				set_eo(ea);
				e = 0;
			}
			else
			{
				set_eo(0);
				e = 0;
			}

			e = get_ei();
			break;

		case neuron_type::invert:
			set_eo(e);
			e = -1.0 * get_ei();
			break;
	}

	/* double eps = 0.01;
	 E -= eps;
	 if (E < 0)
	 E = 0; */

	/* if (E < Ea)
	 E = 0; */
}

/* */
neuron* neuron_rep::get(std::uint64_t id)
{
	size_t sz = neurons.size();
	for (size_t i = 0; i < sz; i++)
	{
		if (neurons[i].c->id == id)
		{
			return &neurons[i];
		}
	}

	return nullptr;
}

/* */
neuron_c* neuron_rep::get_c(std::uint64_t id)
{
	neuron* n = get(id);
	return (n == nullptr) ? nullptr : n->c;
}

/* */
std::uint64_t neuron_rep::insert(const neuron& n)
{
	neurons.push_back(n);

	neuron& nn = neurons.back();
	nn.c->id = id_counter++;
	nn.rep = this;
	return nn.c->id;
}

/* */
void neuron_rep::free(std::uint64_t id)
{
	size_t sz = neurons.size();
	for (size_t i = 0; i < sz; i++)
	{
		if (neurons[i].c->id == id)
		{
			neurons.erase(neurons.begin() + i);
			return;
		}
	}
}

/* */
void neuron_rep::safe_free(std::uint64_t id)
{
	neuron_c* n = get_c(id);

	if (n == nullptr)
	{
		return;
	}

	for (size_t i = 0; i < n->in.size(); i++)
	{
		link* l = link_rep_->get(n->in[i]);
		if (l == nullptr)
		{
			continue;
		}

		l->rep->remove(l->in, l->out);
		i--;
	}

	for (size_t i = 0; i < n->out.size(); i++)
	{
		link* l = link_rep_->get(n->out[i]);
		if (l == nullptr)
		{
			continue;
		}

		l->rep->remove(l->in, l->out);
		i--;
	}

	free(id);
}

/* */
link* link_rep::get(std::uint64_t id)
{
	size_t sz = links.size();
	for (size_t i = 0; i < sz; i++)
	{
		if (links[i].id == id)
		{
			return &links[i];
		}
	}

	return nullptr;
}

/* */
std::uint64_t link_rep::insert(const link& l)
{
	links.push_back(l);

	link& ll = links.back();
	ll.id = id_counter++;
	ll.rep = this;
	return ll.id;
}

/* */
void link_rep::free(std::uint64_t id)
{
	for (size_t i = 0; i < links.size(); i++)
	{
		if (links[i].id == id)
		{
			links.erase(links.begin() + i);
			return;
		}
	}
}

/* */
std::uint64_t link_rep::create(std::uint64_t from, std::uint64_t to, double w)
{
	link* l = get(insert(link())); // TODO: optimize
	l->in = from;
	l->out = to;
	l->w = w;

	neuron_c* n = neuron_rep_->get_c(from);
	if (n != nullptr)
	{
		n->out.push_back(l->id);
	}

	n = neuron_rep_->get_c(to);
	if (n != nullptr)
	{
		n->in.push_back(l->id);
	}

	return l->id;
}

/* */
void link_rep::remove(std::uint64_t from, std::uint64_t to)
{
	for (size_t i = 0; i < links.size(); i++)
	{ // TODO: cache,optimize
		if (links[i].in == from && links[i].out == to)
		{
			link* l = &links[i];
			neuron_c* n = neuron_rep_->get_c(from);
			if (n != nullptr)
			{
				for (size_t j = 0; j < n->out.size(); j++)
				{
					if (n->out[j] == l->id)
					{
						n->out.erase(n->out.begin() + j);
						break;
					}
				}
			}

			n = neuron_rep_->get_c(to);
			if (n != nullptr)
			{
				for (size_t j = 0; j < n->in.size(); j++)
				{
					if (n->in[j] == l->id)
					{
						n->in.erase(n->in.begin() + j);
						break;
					}
				}
			}

			links.erase(links.begin() + i);
			i--;
		}
	}
}

