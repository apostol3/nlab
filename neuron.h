#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

using std::size_t;
namespace nlab{

enum neuron_type
{
	input = 0,
	output,
	blank,
	activ,
	limit,
	binary,
	gen,
	invert
};

class neuron_rep;
class link_rep;
class neuron;

class neuron_c
{
	friend class neuron;
	neuron* _parent;

	double get_ei();
	void set_eo(double eo);

public:
	double e;
	double ea;
	std::uint64_t id;
	neuron_type type;

	std::vector< std::uint64_t > in;
	std::vector< std::uint64_t > out;

	void calc();

	neuron* parent() const
	{
		return _parent;
	}

	neuron_c() : _parent(nullptr), e(0), ea(1), id(0), type(blank)
	{
	}

	neuron_c(const neuron_c& nr): _parent(nr._parent), e(nr.e), ea(nr.ea), id(nr.id), type(nr.type),
		in(nr.in), out(nr.out)
	{
	}

	neuron_c& operator=(const neuron_c& nr)
	{
		e = nr.e;
		ea = nr.ea;
		id = nr.id;
		in = nr.in;
		out = nr.out;
		_parent = nr._parent;
		type = nr.type;
		return *this;
	}
};

class neuron_v
{
	friend class neuron;
	neuron* _parent;

public:
	int x;
	int y;
	int r;

	neuron* parent() const
	{
		return _parent;
	}

	neuron_v() : _parent(nullptr), x(0), y(0), r(20) { }

	neuron_v(const neuron_v& nr): _parent(nr._parent), x(nr.x), y(nr.y), r(nr.r)
	{
	}

	neuron_v& operator=(const neuron_v& nr)
	{
		x = nr.x;
		y = nr.y;
		r = nr.r;
		_parent = nr._parent;
		return *this;
	}
};

class neuron
{
public:
	neuron_c* c;
	neuron_v* v;
	neuron_rep* rep;

	neuron()
	{
		c = new neuron_c();
		c->_parent = this;
		v = new neuron_v();
		v->_parent = this;
		rep = nullptr;
	}

	~neuron()
	{
		delete c;
		delete v;
	}

	neuron(const neuron& nr):c(new neuron_c(*nr.c)), v(new neuron_v(*nr.v))
	{
		c->_parent = this;
		v->_parent = this;
		rep = nr.rep;
	}

	neuron& operator=(const neuron& nr)
	{
		delete c;
		delete v;
		c = new neuron_c(*nr.c);
		c->_parent = this;
		v = new neuron_v(*nr.v);
		v->_parent = this;
		rep = nr.rep;
		return *this;
	}
};

class link
{
public:
	double in_e;
	double out_e;
	double w;
	std::uint64_t in;
	std::uint64_t out;
	std::uint64_t id;

	link_rep* rep;

	void calc()
	{
		out_e = in_e;
		in_e = 0;
	}

	link() : in_e(0), out_e(0), w(0), in(0), out(0), id(0), rep(nullptr) { }

	link(const link& l): in_e(l.in_e), out_e(l.out_e), w(l.w), in(l.in), out(l.out), id(l.id),
		rep(l.rep)
	{
	}

	link& operator=(const link& l)
	{
		in = l.in;
		out = l.out;
		w = l.w;
		id = l.id;
		rep = l.rep;
		in_e = l.in_e;
		out_e = l.out_e;
		return *this;
	}
};

class neuron_rep
{
public:
	neuron* get(std::uint64_t id);
	neuron_c* get_c(std::uint64_t id);
	std::uint64_t insert(const neuron& n);
	void free(std::uint64_t id);
	void safe_free(std::uint64_t id);

	std::vector< neuron > neurons;
	std::uint64_t id_counter;
	link_rep* link_rep_;

	neuron_rep() : link_rep_(nullptr)
	{
		id_counter = 1;
	}

	neuron_rep& operator=(const neuron_rep& nr)
	{
		neurons.clear();
		id_counter = nr.id_counter;
		link_rep_ = nr.link_rep_;
		neurons.reserve(nr.neurons.size());
		for (size_t i = 0; i < nr.neurons.size(); i++)
		{
			neurons.push_back(nr.neurons[i]);
			neurons[i].rep = this;
		}

		return *this;
	}

	neuron_rep(const neuron_rep& nr): neurons(nr.neurons), id_counter(nr.id_counter),
		link_rep_(nr.link_rep_)
	{
		for (size_t i = 0; i < nr.neurons.size(); i++)
		{
			neurons[i].rep = this;
		}
	}
};

class link_rep
{
public:
	link* get(std::uint64_t id);
	std::uint64_t insert(const link& l);
	void free(std::uint64_t id);
	std::uint64_t create(std::uint64_t from, std::uint64_t to, double w);
	void remove(std::uint64_t from, std::uint64_t to);

	std::vector< link > links;
	std::uint64_t id_counter;
	neuron_rep* neuron_rep_;

	link_rep() : neuron_rep_(nullptr)
	{
		id_counter = 1;
	}

	link_rep& operator=(const link_rep& lr)
	{
		links.clear();
		id_counter = lr.id_counter;
		neuron_rep_ = lr.neuron_rep_;
		links.reserve(lr.links.size());
		for (size_t i = 0; i < lr.links.size(); i++)
		{
			links.push_back(lr.links[i]);
			links[i].rep = this;
		}

		return *this;
	}

	link_rep(const link_rep& lr):links(lr.links), id_counter(lr.id_counter),
		neuron_rep_(lr.neuron_rep_)
	{
		for (size_t i = 0; i < lr.links.size(); i++)
		{
			links[i].rep = this;
		}
	}
};

};