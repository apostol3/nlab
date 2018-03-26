#include <iostream>
#include <benchmark/benchmark.h>
#include "tweann.h"
#include "neuron.h"
#include "tcp_stream.h"
#include "json_routines.h"

using namespace nlab;

class net_simple: public benchmark::Fixture
{
public:
	void SetUp(const benchmark::State& state)
	{
		net = json_routines::load_from_file(std::string("test1.nnt"));
	}

	void TearDown(const benchmark::State& state)
	{
		delete net;
	}

	tweann* net;
};

class net_complex : public benchmark::Fixture
{
public:
	void SetUp(const benchmark::State& state)
	{
		net = json_routines::load_from_file(std::string("test2.nnt"));
	}

	void TearDown(const benchmark::State& state)
	{
		delete net;
	}

	tweann* net;
};

static void tweann_construct(benchmark::State& state)
{
	const size_t in = state.range_x();
	const size_t out = state.range_y();
	while (state.KeepRunning())
	{
		tweann net(in, out);
		benchmark::DoNotOptimize(net);
	}
	state.SetItemsProcessed(state.iterations());
}

static void neuron_construct(benchmark::State& state)
{
	while (state.KeepRunning())
	{
		neuron n;
		benchmark::DoNotOptimize(n);
	}
	state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(net_complex, neuron_rep_get)(benchmark::State& state)
{
	std::vector<size_t> ids;
	for (auto i: net->nr.neurons)
		ids.push_back(i.c->id);

	const auto begin = ids.begin();
	const auto end = ids.end();
	auto cur = begin;
	while (state.KeepRunning())
	{
		if (++cur == end)
			cur = begin;
		benchmark::DoNotOptimize(net->nr.get(*cur));
	}
	state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(net_complex, link_rep_get)(benchmark::State& state)
{
	std::vector<size_t> ids;
	for (auto i : net->lr.links)
		ids.push_back(i.id);

	const auto begin = ids.begin();
	const auto end = ids.end();
	auto cur = begin;
	while (state.KeepRunning())
	{
		if (++cur == end)
			cur = begin;
		benchmark::DoNotOptimize(net->nr.get(*cur));
	}
	state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(net_complex, tweann_calc)(benchmark::State& state)
{
	net_task input;
	input.resize(13, 1);

	while (state.KeepRunning())
	{
		benchmark::DoNotOptimize(net->calc(input));
	}
	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations()*(13 * sizeof(double)));
}

BENCHMARK_DEFINE_F(net_simple, tweann_calc)(benchmark::State& state)
{
	net_task input;
	input.resize(13, 1);

	while (state.KeepRunning())
	{
		benchmark::DoNotOptimize(net->calc(input));
	}
	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations()*(13 * sizeof(double)));
}

BENCHMARK_DEFINE_F(net_complex, tweann_reset)(benchmark::State& state)
{
	while (state.KeepRunning())
	{
		benchmark::DoNotOptimize(net->reset());
	}
	state.SetItemsProcessed(state.iterations());
}

static void handle_json_message_check(benchmark::State& state)
{
	const size_t udp_buffer = 307200;
	const std::string udp_port = "13551";
	tcp_stream stream(udp_buffer);
	stream.create(udp_port);

	while (state.KeepRunning())
	{
		void* p_str = nullptr;
		size_t sz;
		stream.receive(&p_str, sz);

		if (sz != 0)
		{
			std::cerr << "received something, aborting" << std::endl;
			return;
		}
	}
	state.SetItemsProcessed(state.iterations());
}

static void load_net_from_file(benchmark::State& state)
{
	std::ostringstream ss;
	ss << "test" << state.range_x() << ".nnt";
	const std::string file_name(ss.str());

	while (state.KeepRunning())
	{
		tweann* net = json_routines::load_from_file(file_name);
		
		state.PauseTiming();
		delete net;
		state.ResumeTiming();
	}
	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations() * (state.range_x()==1?19145: 46936));
}

static void load_net_from_string(benchmark::State& state)
{
	std::ostringstream ss;
	ss << "test" << state.range_x() << ".nnt";
	const std::string file_name(ss.str());
	std::ifstream sv(file_name, std::ios_base::in);
	if (sv.fail())
	{
		throw std::runtime_error("Open failed. Couldn't open file");
	}

	std::string buf;
	while (!sv.eof())
	{
		char a;
		sv.read(&a, 1);
		buf.push_back(a);
	}

	buf.pop_back();
	while (state.KeepRunning())
	{
		tweann* net = json_routines::load_from_string(buf);
		state.PauseTiming();
		delete net;
		state.ResumeTiming();
	}
	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations() * buf.size());
}

void dump_net_to_string(benchmark::State& state)
{
	std::ostringstream ss;
	ss << "test" << state.range_x() << ".nnt";
	const std::string file_name(ss.str());
	tweann* net = json_routines::load_from_file(file_name);
	std::string st;
	while (state.KeepRunning())
	{
		state.PauseTiming();
		st.clear();
		state.ResumeTiming();
		st = json_routines::dump_to_string(net);

	}
	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations() * st.size());
	delete net;
}

BENCHMARK(neuron_construct);
BENCHMARK(tweann_construct)->ArgPair(3, 1)->ArgPair(10, 10)->ArgPair(100, 100);
BENCHMARK_REGISTER_F(net_complex, neuron_rep_get);
BENCHMARK_REGISTER_F(net_complex, link_rep_get);
BENCHMARK_REGISTER_F(net_complex, tweann_calc);
BENCHMARK_REGISTER_F(net_simple, tweann_calc);
BENCHMARK_REGISTER_F(net_complex, tweann_reset);
BENCHMARK(handle_json_message_check);
BENCHMARK(load_net_from_file)->Arg(1)->Arg(2);
BENCHMARK(load_net_from_string)->Arg(1)->Arg(2);
BENCHMARK(dump_net_to_string)->Arg(1)->Arg(2);

BENCHMARK_MAIN();

