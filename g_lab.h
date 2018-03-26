#pragma once

#include "tweann.h"
#include "env.h"
namespace nlab
{

	class g_lab
	{
	public:
		g_lab()
		{
		}

		void generate_neuron(tweann* nt);
		void delete_neuron(tweann* nt);
		void change_weight(tweann* nt);
		void make_link(tweann* nt);
		void delete_link(tweann* nt);
		void change_activation(tweann* nt);
		void change_neuron_type(tweann* nt);
		void mutate(tweann* nt);
		void full_mutate(tweann* nt);

		void fitness_sort(std::vector< tweann * >& nts);
		static bool pn_sort(tweann* a, tweann* b);
		void pop_gen(std::vector< tweann * >& nts, size_t popsize, size_t in, size_t out);
		void pop_mutate(std::vector< tweann * >& nts);
		void pop_mutate_1(std::vector< tweann * >& nts);
		int gen_cycle(std::vector< tweann * >& nts, base_env* env, size_t& cps);
		int cycle(std::vector< tweann * >& nts, base_env* env, size_t popsize, size_t& cps);
	private:
		static size_t random();
		static size_t random(size_t max);
		static int random(int max);
		static double random(double max);
	};

	struct callback_info
	{
		size_t* cps;
		net_task* in;
		net_task* out;
		tweann* net;
		size_t count;
	};

};