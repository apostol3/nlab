#include <stdio.h>
#include <iostream>
#include <locale>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <codecvt>
#include <memory>

#include "tcp_stream.h"
#include "remote_env.h"
#include "g_lab.h"
#include "tweann.h"
#include "json_routines.h"

#ifdef WIN32
#include "pipe_stream.h"
#endif

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/encodedstream.h>

#include "json_rpc_server.h"

using namespace nlab;

using namespace std;

void create_folder(std::string name)
{
#ifdef WIN32
#include <windows.h>
	CreateDirectoryA(name.c_str(), nullptr);
#else
#include <sys/stat.h>
#include <sys/types.h>
	mkdir(name.c_str(), S_IRWXU);
#endif
}

struct nlab_worker
{
	enum
	{
		stopped,
		running,
		paused
	} state = stopped;

	remote_env env;
	g_lab gl;
	std::vector< tweann * > nts;
	size_t cps = 0;
	size_t popsize = 10;

	size_t last_speed = 0;
	double last_best = 0;

	size_t rounds = 1;
	size_t cur_round = 0;

	std::string save_dir;
	const std::string save_dir_prefix = "nets/";
	bool save_dir_changed = false;

	std::string connection_uri = "tcp://127.1:5005";

	size_t ticks = 0, max_ticks = 0;

	nlab_worker() : env(std::make_unique<tcp_stream>(307200))
	{
	}

	void teach();
	void do_idle();

	std::string get_save_dir() const
	{
		return save_dir_prefix + save_dir + "/";
	}

	static std::string generate_save_dir()
	{
		ostringstream os;
		os << "untitled" << std::chrono::system_clock::now().time_since_epoch().count();
		return os.str();
	}
} worker;

const size_t udp_buffer = 307200;
const size_t idle_update_time = 100;
const size_t worker_update_time = 300;

tcp_stream udp_pipe(udp_buffer);
jsonrpc::server json_server;

void handle_json_message()
{
	void* p_str = nullptr;
	size_t sz;
	udp_pipe.receive(&p_str, sz);

	if (sz != 0)
	{
		std::string received = {static_cast< char* >(p_str)};
		std::cout << "Received: " << (received.size() > 1000 ? "<too long to print>" : received)
			<< std::endl;

		std::string reply = json_server.handle(received);
		if (!reply.empty())
		{
			std::cout << "Sended: " << (reply.size() > 1000 ? "<too long to print>" : reply)
				<< std::endl;
			udp_pipe.send(reply.c_str(), reply.size() + 1);
		}
	}
}

rapidjson::Value method_get_state(const rapidjson::Value& /* params */, const rapidjson::Value& /* id */,
                                  rapidjson::MemoryPoolAllocator< >& allocator)
{
	rapidjson::Value res;
	std::wstring_convert< std::codecvt_utf8_utf16< wchar_t > > converter;

	res.SetObject();
	res.AddMember("speed", static_cast<uint64_t>(worker.last_speed), allocator);
	res.AddMember("best", worker.last_best, allocator);
	res.AddMember("max_round", static_cast<uint64_t>(worker.rounds), allocator);
	res.AddMember("round", static_cast<uint64_t>(worker.cur_round), allocator);
	res.AddMember("popsize", static_cast<uint64_t>(worker.popsize), allocator);
	res.AddMember("save_dir",
	              rapidjson::Value().SetString(worker.save_dir.c_str(), allocator),
	              allocator);

	res.AddMember("env_uri", rapidjson::Value().SetString(
		worker.connection_uri.c_str(), allocator), allocator);

	switch (worker.state)
	{
		case nlab_worker::stopped: res.AddMember("state", "stopped", allocator);
			break;
		case nlab_worker::running: res.AddMember("state", "running", allocator);
			break;
		case nlab_worker::paused: res.AddMember("state", "paused", allocator);
			break;
	}
	return res;
}

rapidjson::Value method_get_population(const rapidjson::Value& /* params */, const rapidjson::Value& /* id */,
                                       rapidjson::MemoryPoolAllocator< >& allocator)
{
	if (worker.state == nlab_worker::stopped)
	{
		throw new jsonrpc::exceptions::method_error("server stopped", 2);
	}

	rapidjson::Value arr;
	arr.SetArray();

	for (auto i : worker.nts)
	{
		std::string string = json_routines::dump_to_string(i);
		rapidjson::Document doc;
		doc.Parse(string.c_str());
		rapidjson::Value value(doc, allocator);
		arr.PushBack(value, allocator);
	}

	rapidjson::Value res;
	res.SetObject();
	res.AddMember("nets", arr, allocator);

	return res;
}

rapidjson::Value method_stop(const rapidjson::Value& /* params */, const rapidjson::Value& /* id */,
                             rapidjson::MemoryPoolAllocator< >&)
{
	if (worker.state == nlab_worker::stopped)
	{
		throw new jsonrpc::exceptions::method_error("server is already stopped", 2);
	}

	worker.state = nlab_worker::stopped;
	return rapidjson::Value();
}

rapidjson::Value method_start(const rapidjson::Value& params, const rapidjson::Value& /* id */,
                              rapidjson::MemoryPoolAllocator< >&)
{
	if (worker.state != nlab_worker::stopped)
	{
		throw new jsonrpc::exceptions::method_error("server isn't stopped", 1);
	}

	worker.max_ticks = 0;
	worker.ticks = 0;

	if (params.IsObject())
	{
		if (params.HasMember("save_dir") && params["save_dir"].IsString())
		{
			worker.save_dir = params["save_dir"].GetString();
			worker.save_dir_changed = true;
		}

		if (params.HasMember("popsize") && params["popsize"].IsUint())
			worker.popsize = params["popsize"].GetUint();

		if (params.HasMember("rounds") && params["rounds"].IsUint())
			worker.rounds = params["rounds"].GetUint();

		if (params.HasMember("ticks") && params["ticks"].IsUint())
			worker.max_ticks = params["ticks"].GetUint();

		if (params.HasMember("env_uri") && params["env_uri"].IsString())
		{
			worker.connection_uri = params["env_uri"].GetString();
		}
	}

	worker.state = nlab_worker::running;

	return rapidjson::Value();
}

rapidjson::Value method_resume(const rapidjson::Value& params, const rapidjson::Value& /* id */,
                               rapidjson::MemoryPoolAllocator< >&)
{
	if (worker.state != nlab_worker::paused)
	{
		throw new jsonrpc::exceptions::method_error("server isn't paused", 3);
	}

	worker.max_ticks = 0;
	worker.ticks = 0;

	if (params.IsObject())
	{
		if (params.HasMember("ticks") && params["ticks"].IsUint())
			worker.max_ticks = params["ticks"].GetUint();
	}

	worker.state = nlab_worker::running;
	cout << "Resumed" << endl;
	return rapidjson::Value();
}

rapidjson::Value method_pause(const rapidjson::Value& /* params */, const rapidjson::Value& /* id */,
                              rapidjson::MemoryPoolAllocator< >&)
{
	if (worker.state != nlab_worker::running)
	{
		throw new jsonrpc::exceptions::method_error("server isn't running", 4);
	}

	worker.state = nlab_worker::paused;
	cout << "Paused" << endl;
	return rapidjson::Value();
}

void nlab_worker::do_idle()
{
	auto old_state = worker.state;
	while (worker.state == old_state)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(idle_update_time));
		handle_json_message();
	}
}

void nlab_worker::teach()
{
	cur_round = 0;
	last_best = 0;
	last_speed = 0;

	if (!worker.save_dir_changed)
		worker.save_dir = worker.generate_save_dir();

	create_folder(worker.get_save_dir());

	cout << "Connecting...";

	auto colon_ind = connection_uri.find("://");
	if (colon_ind == std::string::npos)
		throw std::invalid_argument("couldn't parse connection URI");

	auto uri_net_part = connection_uri.substr(colon_ind + 3);
	auto scheme = connection_uri.substr(0, colon_ind);

	if (scheme == "tcp")
	{
		auto port_ind = uri_net_part.find(":");
		if (port_ind == std::string::npos)
			throw std::invalid_argument("couldn't parse connection URI");
		env = remote_env( std::make_unique<tcp_stream>(uri_net_part.substr(0, port_ind),
			uri_net_part.substr(port_ind + 1), 307200 ));
	}
	else if (scheme == "winpipe")
	{

#ifdef WIN32
		auto slash_ind = uri_net_part.find("/");
		if (slash_ind == std::string::npos)
			throw std::invalid_argument("couldn't parse connection URI");
		std::cout << "winpipes: " << uri_net_part.substr(slash_ind + 1) << std::endl;
		env = remote_env( std::make_unique<pipe_stream>(uri_net_part.substr(slash_ind + 1).c_str(),
			307200, 307200 ) );
#else
		throw std::runtime_error("winpipe not avaliable on this platform");
#endif
	}
	else throw std::invalid_argument("unknown connection URI scheme");

	env.init();
	cout << " done\n";

	cout << "Starting...";

	e_start_info esinf = env.get_start_info();
	n_start_info nsinf;
	nsinf.count = esinf.count ? esinf.count : 1;

	nsinf.round_seed = std::chrono::system_clock::now().time_since_epoch().count();
	env.set_start_info(nsinf);
	cout << " done\n";

	while (1)
	{
		cout << "Round " << cur_round << " of " << rounds << " started\n";
		if (cur_round >= rounds && rounds > 0)
		{
			break;
		}

		try
		{
			if (gl.cycle(nts, &env, popsize, cps) != 0)
			{
				break;
			}
		}
		catch (std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			break;
		}

		last_best = nts.front()->fitness;

		if (nts.size() > 0 && nts[0]->fitness >= 30000)
		{
			cout << "Solved on " << cur_round << " round\n";
			break;
		}

		cout << "Round: " << cur_round << " ended\n";

		std::ostringstream os;
		os << get_save_dir() << "round " << cur_round << ".nnt";
		try
		{
			json_routines::dump_to_file(nts[0], os.str());
		}
		catch (exception& e)
		{
			std::cerr << e.what() << std::endl;
		}

		cur_round++;
	}

	cout << "Stopping...";
	worker.state = stopped;

	try
	{
		e_send_info eseinf = env.get();
		env.stop();
	}
	catch (exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	cout << " done\n";
}

/* */
int main(int argc, char* argv[])
{
	int port = 13550;

	if (argc > 1)
	{
		if (std::strcmp(argv[1], "--help") == 0)
		{
			std::cout << "nlab worker" << std::endl;
			std::cout << "usage: " << argv[0] << " [port (default: 13550)] " <<
				"[connection_uri (default: 'tcp://127.0.0.1:5005')] [net_file]" << std::endl;
			return 0;
		}
		try
		{
			port = std::stoi(argv[1]);
			if (port < 1 || port>65535)
				throw std::invalid_argument("port must be in [1,65535]");
		}
		catch (std::invalid_argument& e)
		{
			std::cerr << "invalid port: " << e.what() << std::endl;
			return -1;
		}
	}

	if (argc > 2)
	{
		worker.connection_uri = argv[2];
	}

	if (argc > 3)
	{
		tweann* nt;
		try
		{
			nt = json_routines::load_from_file(argv[3]);
		}
		catch (exception& e)
		{
			std::cerr << "couldn't open net file: " << e.what() << std::endl;
			return -1;
		}

		worker.nts.push_back(nt);
	}


	//BUG: dir will change later
	if (!worker.save_dir_changed)
		worker.save_dir = worker.generate_save_dir();

	create_folder(worker.save_dir_prefix);



	json_server.add_method("get_state", method_get_state);
	json_server.add_method("get_population", method_get_population);
	json_server.add_method("stop", method_stop);
	json_server.add_method("start", method_start);
	json_server.add_method("pause", method_pause);
	json_server.add_method("resume", method_resume);

	std::cout << "Creating at port: " << port << "...";
	try
	{
		udp_pipe.create(to_string(port));
	}
	catch (std::exception& e)
	{
		std::cerr << e.what();
		return -1;
	}

	std::cout << " done\n";

	try
	{
		while (1)
		{
			worker.do_idle();
			worker.teach();
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what();
		return -1;
	}

	return 0;
}

/* */
int g_Callback(callback_info /* nf */)
{
	static std::chrono::high_resolution_clock::time_point now, t1, t2;

	worker.ticks++;
	if (worker.max_ticks && worker.ticks > worker.max_ticks)
	{
		worker.state = nlab_worker::paused;
		cout << "Paused" << endl;
		worker.do_idle();
		if (worker.state == nlab_worker::stopped)
			return -1;
	}

	now = std::chrono::high_resolution_clock::now();
	if (std::chrono::duration_cast < std::chrono::milliseconds > (now - t2).count() > worker_update_time)
	{
		handle_json_message();
		
		t2 = std::chrono::high_resolution_clock::now();

		if (worker.state == nlab_worker::paused)
			worker.do_idle();
		if (worker.state == nlab_worker::stopped)
			return -1;
	}

	if (std::chrono::duration_cast < std::chrono::milliseconds > (now - t1).count() > 1000)
	{
		worker.cps = double(worker.cps) / (std::chrono::duration_cast < std::chrono::milliseconds > (now - t1).count()/1000.0);
		t1 = std::chrono::high_resolution_clock::now();
		cout << "Speed: " << worker.cps << " p/s\n";
		worker.last_speed = worker.cps;
		worker.cps = 0;
	}

	return 0;
}

