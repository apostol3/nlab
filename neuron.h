#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>


namespace nlab{

using std::size_t;

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
	neuron* _parent{nullptr};

	double get_ei();
	void set_eo(double eo);

public:
	double e{0.0};
	double ea{1};
	std::uint64_t id{0};
	neuron_type type{blank};

	std::vector< std::uint64_t > in;
	std::vector< std::uint64_t > out;

	void calc();

	neuron* parent() const
	{
		return _parent;
	}

};

class neuron_v
{
	friend class neuron;
	neuron* _parent{nullptr};

public:
	int x{0};
	int y{0};
	int r{20};

	neuron* parent() const
	{
		return _parent;
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

	neuron(const neuron& nr):
		c(new neuron_c(*nr.c)),
		v(new neuron_v(*nr.v)),
		rep(nr.rep)
	{
		c->_parent = this;
		v->_parent = this;
	}

	neuron& operator=(const neuron& nr)
	{
		if (&nr == this)
		{
			return *this;
		}

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
	double in_e{0.0};
	double out_e{0.0};
	double w{0.0};
	std::uint64_t in{0};
	std::uint64_t out{0};
	std::uint64_t id{0};

	link_rep* rep{nullptr};

	void calc()
	{
		out_e = in_e;
		in_e = 0;
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
	std::uint64_t id_counter{1};
	link_rep* link_rep_{nullptr};

	neuron_rep() = default;

	neuron_rep& operator=(const neuron_rep& nr)
	{
		if (&nr == this)
		{
			return *this;
		}

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
	std::uint64_t id_counter{1};
	neuron_rep* neuron_rep_{nullptr};

	link_rep() = default;

	link_rep& operator=(const link_rep& lr)
	{
		if (&lr == this)
		{
			return *this;
		}

		
		id_counter = lr.id_counter;
		neuron_rep_ = lr.neuron_rep_;
		links = lr.links;

		for (auto& i: links)
		{
			i.rep = this;
		}

		return *this;
	}

	link_rep(const link_rep& lr):links(lr.links), id_counter(lr.id_counter),
		neuron_rep_(lr.neuron_rep_)
	{
		for (auto& i : links)
		{
			i.rep = this;
		}
	}
};

} // namespace nlab