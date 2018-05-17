#include <algorithm>
#include <iostream>
#include <chrono>
#include <random>

#include "g_lab.h"

using namespace nlab;

size_t g_lab::random()
{
	static std::uint64_t seed = std::chrono::system_clock::now().time_since_epoch().count();
	static std::mt19937_64 gen(seed);
	static std::uniform_int_distribution< std::uint32_t > dist; //borland compiler fails with 
	//uint64_t distribution
	return (std::uint64_t(dist(gen)) << 32) | std::uint64_t(dist(gen));
}

size_t g_lab::random(size_t max)
{
	return random() % max;
}

int g_lab::random(int max)
{
	return random() % max;
}

double g_lab::random(double max)
{
	static std::uint64_t seed = std::chrono::system_clock::now().time_since_epoch().count();
	static std::mt19937_64 gen(seed);
	static std::uniform_real_distribution< double > dist(0, 1);
	return dist(gen) * max;
}

bool AcceptList[8] = {0, 0, 1, 1, 0, 1, 1, 1}; // TODO: bring into class

/* */
void g_lab::generate_neuron(tweann* nt)
{
	if (nt == nullptr)
	{
		throw;
	}

	if (nt->nr.neurons.size() > 150) //TODO: extract constant as class property
	{
		return;
	}

	int count = 1;
	const int visc = 1;
	for (int i = 0; i < count; i++)
	{
		neuron_c* N = nt->nr.get_c(nt->nr.insert(neuron())); //TODO: try optimize
		neuron_v* V = N->parent()->v;
		int rt = -1;
		int j = 0;
		while (rt == -1)
		{
			rt = random(6) + 2;
			if (!AcceptList[rt])
			{ //TODO: Cache list out of while
				rt = -1;
			}

			j++;
			if (j > 100)
			{
				nt->nr.free(N->id);

				return;
			}
		}

		N->type = static_cast< neuron_type >(rt);
		V->x = random(400);
		V->y = random(400);
		V->r = 20;

		for (int k = 0; k < visc; k++)
		{
			size_t ri = 0;
			size_t ro = 0;
			while
			(
				ri == ro ||
				nt->nr.neurons[ri].c->type == neuron_type::output ||
				nt->nr.neurons[ro].c->type == neuron_type::input
			)
			{
				ri = random(nt->nr.neurons.size() - 1);
				ro = random(nt->nr.neurons.size() - 1);
			}

			nt->lr.create(nt->nr.neurons[ri].c->id, N->id, random(1000) / 1000.0);
			nt->lr.create(N->id, nt->nr.neurons[ro].c->id, random(1000) / 1000.0);
		}
	}
}

/* */
void g_lab::delete_neuron(tweann* nt)
{
	if (nt == nullptr)
	{
		throw;
	}

	neuron_c* L = nullptr;
	int i = 0;
	size_t sz = nt->nr.neurons.size();
	while (L == nullptr)
	{
		L = nt->nr.neurons[random(sz)].c;
		if (L->type == neuron_type::input || L->type == neuron_type::output)
		{
			L = nullptr;
		}

		if (++i > 100)
		{
			return;
		}
	}

	nt->nr.safe_free(L->id);
}

/* */
void g_lab::change_weight(tweann* nt)
{
	if (nt == nullptr)
	{
		throw;
	}

	if (nt->lr.links.empty())
	{
		return;
	}

	double& lw = nt->lr.links[random(nt->lr.links.size())].w;
	if (lw > 0)
	{
		if (lw < 10)
		{
			lw *= (random(4145) + 8000) / 10000.0;
		}
		else
		{
			lw *= (random(2000) + 8000) / 10000.0;
		}
	}
	else
	{
		lw = random(150) / 1000.0;
	}
}

/* */
void g_lab::change_activation(tweann* nt)
{
	if (nt == nullptr)
	{
		throw;
	}

	double& ea = nt->nr.neurons[random(nt->nr.neurons.size())].c->ea;
	if (ea > 0)
	{
		if (ea < 10)
		{
			ea *= (random(4145) + 8000) / 10000.0;
		}
		else
		{
			ea *= (random(2000) + 8000) / 10000.0;
		}
	}
	else
	{
		ea = random(150) / 1000.0;
	}
}

/* */
void g_lab::make_link(tweann* nt)
{
	if (nt == nullptr)
	{
		throw;
	}

	size_t ri = 0;
	size_t ro = 0;
	unsigned i = 0;
	while
	(
		ri == ro ||
		nt->nr.neurons[ri].c->type == neuron_type::output ||
		nt->nr.neurons[ro].c->type == neuron_type::input
	)
	{
		ri = random(nt->nr.neurons.size());
		ro = random(nt->nr.neurons.size());
		for (size_t j = 0; j < nt->lr.links.size(); j++)
		{
			if (nt->lr.links[j].in == nt->nr.neurons[ri].c->id &&
				nt->lr.links[j].out == nt->nr.neurons[ro].c->id)
			{
				ri = 0;
				ro = 0;
				break;
			}
		}

		if (++i > 100)
		{
			return;
		}
	}

	nt->lr.create(nt->nr.neurons[ri].c->id, nt->nr.neurons[ro].c->id, random(1000) / 1000.0);
}

/* */
void g_lab::delete_link(tweann* nt)
{
	if (nt == nullptr)
	{
		throw;
	}

	if (nt->lr.links.empty())
	{
		return;
	}

	size_t rnd = random(nt->lr.links.size());
	nt->lr.remove(nt->lr.links[rnd].in, nt->lr.links[rnd].out);
}

/* */
void g_lab::change_neuron_type(tweann* nt)
{
	if (nt == nullptr)
	{
		throw;
	}

	size_t rnd;
	size_t i = 0;
	size_t sz = nt->nr.neurons.size();
	while (true)
	{
		rnd = random(sz);
		neuron_type& tp = nt->nr.neurons[rnd].c->type;
		if (tp != neuron_type::input && tp != neuron_type::output)
		{
			break;
		}

		if (++i > 100)
		{
			return;
		}
	}

	int rt = -1;
	i = 0;
	while (rt == -1)
	{
		rt = random(6) + 2;
		if (AcceptList[rt] == false)
		{ //TODO: get list out of while
			rt = -1;
		}

		if (++i > 100)
		{
			return;
		}
	}

	nt->nr.neurons[rnd].c->type = static_cast< neuron_type >(rt);
}

/* */
void g_lab::mutate(tweann* nt)
{
	if (nt == nullptr)
	{
		throw;
	}

	int a = random(1000);
	if (a < 70) //TODO: extract magic numbers as class properties
	{
		generate_neuron(nt);
	}
	else if (a < 140)
	{
		delete_neuron(nt);
	}
	else if (a < 260)
	{
		make_link(nt);
	}
	else if (a < 380)
	{
		delete_link(nt);
	}
	else if (a < 680)
	{
		change_weight(nt);
	}
	else if (a < 880)
	{
		change_activation(nt);
	}
	else
	{
		change_neuron_type(nt);
	}
}

/* */
void g_lab::full_mutate(tweann* nt)
{
	for (int i = 0; i < 1000; i++)
	{
		mutate(nt);
	}
}

/* */
bool g_lab::pn_sort(tweann* a, tweann* b)
{
	return a->fitness > b->fitness;
}

/* */
void g_lab::fitness_sort(std::vector< tweann * >& nts)
{
	std::sort(nts.begin(), nts.end(), g_lab::pn_sort);
}

/* */
void g_lab::pop_mutate(std::vector< tweann * >& nts)
{
	for (size_t i = nts.size() / 2; i < nts.size(); i++)
	{
		delete nts[i];
		nts[i] = new tweann(*nts[i - nts.size() / 2]);
		nts[i]->fitness = 0;
		mutate(nts[i]);
		if (random(500) > 498) //TODO: extract constant as class property
		{
			full_mutate(nts[i]);
		}
	}
}

/* */
void g_lab::pop_mutate_1(std::vector< tweann * >& nts)
{
	double smax = 0;
	for (size_t i = 0; i < nts.size() / 2; i++)
	{
		smax += nts[i]->fitness;
	}

	for (size_t i = nts.size() / 2; i < nts.size(); i++)
	{
		delete nts[i];

		double rnd = random(smax) + 1;
		size_t j = 0;
		double sum = 0;
		while (sum < rnd && j < nts.size() / 2 - 1)
		{
			sum += nts[j]->fitness;
			j++;
		}

		j--;
		nts[i] = new tweann(*nts[j]);
		nts[i]->fitness = 0;
		mutate(nts[i]);
		if (random(500) > 498)
		{
			full_mutate(nts[i]);
		}
	}
}

extern int g_Callback(callback_info nf);

/* */
int g_lab::gen_cycle(std::vector< tweann * >& nts, base_env* env, size_t& cps)
{
	size_t cnt = env->get_state().count;
	cnt = (cnt == 0) ? cnt : 1;

	for (size_t i = 0; i < nts.size(); i++)
	{
		std::vector< tweann * > ntt;

		for (size_t j = 0; j < cnt; j++)
		{
			if (i >= nts.size())
			{
				ntt.push_back(nullptr);
				continue;
			}

			ntt.push_back(nts[i]);
			i++;
			ntt.back()->reset();
			ntt.back()->fitness = 0;
		}

		i--;

		/* if(nt->fts!=0&&urandom(1000)<750)
		 continue; */
		while (true)
		{
			e_send_info esinf;
			esinf = env->get();

			if (esinf.head == verification_header::restart)
			{
				e_restart_info erinf = env->get_restart_info();

				if (erinf.result.size() != cnt)
				{
					throw std::runtime_error("Internal error:\nerinf.result.size() != cnt");
					return -1;
				}

				for (size_t k = 0; k < ntt.size(); k++)
				{
					if (ntt[k] == nullptr)
					{
						break;
					}

					ntt[k]->fitness = erinf.result[k];
				}
				break;
			}

			if (esinf.head == verification_header::stop)
			{
				return -1;
			}

			if (esinf.data.size() != cnt)
			{
				throw std::runtime_error("Internal error:\nesinf.data.size() != cnt");
			}

			std::vector< env_task > rsr;
			net_task In;
			net_task Out;
			for (size_t k = 0; k < ntt.size(); k++)
			{
				tweann* nt = ntt[k];
				In = esinf.data[k];
				if (nt == nullptr || In.empty())
				{
					Out.clear();
					for (unsigned int z = 0; z < env->get_state().outcount; z++)
					{
						Out.push_back(0);
					}

					rsr.push_back(Out);
					continue;
				}

				if (In.size() != env->get_state().incount)
				{
					throw std::runtime_error(
						"Internal error:\nIn.size()!=env->GetState().incount");
				}

				cps++;
				nt->fitness++;
				Out = nt->calc(In);

				if (Out.size() != env->get_state().outcount)
				{
					throw std::runtime_error(
						"Internal error:\nOut.size() != env->GetState().outcount");
				}

				rsr.push_back(Out);
			}

			n_send_info nsinf;
			nsinf.head = verification_header::ok;
			nsinf.data = rsr;
			env->set(nsinf);

			callback_info nf;
			nf.cps = &cps;
			nf.in = &esinf.data.front();
			nf.out = &nsinf.data.front();
			nf.net = ntt.front();
			nf.count = cnt;
			if (g_Callback(nf) != 0)
			{
				return -1;
			}
		}

		n_restart_info nrinf;
		nrinf.count = cnt;
		if (i + 1 < nts.size())
		{
			nrinf.round_seed = env->get_state().round_seed;
		}
		else 
		{
			nrinf.round_seed = random();
		}

		env->restart(nrinf);
		
	}

	return 0;
}

/* */
void g_lab::pop_gen(std::vector< tweann * >& nts, size_t popsize, size_t in, size_t out)
{
	while (nts.size() > popsize)
	{
		delete nts.back();
		nts.pop_back();
	}

	while (nts.size() < popsize)
	{
		nts.push_back(new tweann(in, out));
		//it maybe also need to pre-init net; like full_mutate(nts)
	}
}

/* */
int g_lab::cycle(std::vector< tweann * >& nts, base_env* env, size_t popsize, size_t& cps)
{
	if (popsize < 2)
	{
		throw std::runtime_error("Population size < 2!");
	}

	pop_gen(nts, popsize, env->get_state().incount, env->get_state().outcount);

	if (gen_cycle(nts, env, cps) != 0)
	{
		return -1;
	}

	fitness_sort(nts);

	std::cout << "Best: " << nts[0]->fitness << "\n";

	double avfts = 0;
	for (auto& i : nts)
	{
		avfts += i->fitness;
	}

	avfts /= static_cast< double >(nts.size());
	std::cout << "Average: " << avfts << "\n";

	pop_mutate(nts);

	return 0;
}

