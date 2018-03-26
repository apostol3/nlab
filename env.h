#pragma once

#include <vector>

typedef std::vector< double > env_task;

enum class verification_header
{
	ok = 0,
	restart,
	stop,
	fail
};

enum class send_modes
{
	specified = 0,
	undefined
};

struct e_start_info
{
	send_modes mode;
	size_t count;
	size_t incount;
	size_t outcount;

	e_start_info() : mode(send_modes::specified), count(0), incount(0), outcount(0) { }
};

struct n_start_info
{
	size_t count;
	size_t round_seed;

	n_start_info() : count(0), round_seed(0) { }
};

struct n_send_info
{
	verification_header head;

	std::vector< env_task > data;

	n_send_info() : head(verification_header::fail) { }
};

struct e_send_info
{
	verification_header head;

	std::vector< env_task > data;

	e_send_info() : head(verification_header::fail)
	{
		head = verification_header::fail;
	}
};

struct n_restart_info
{
	size_t count;
	size_t round_seed;

	n_restart_info() : count(0), round_seed(0)
	{
	}
};

struct e_restart_info
{
	std::vector< double > result;

	e_restart_info()
	{
		result.push_back(-1);
	}
};

struct env_state
{
	send_modes mode;
	size_t count;
	size_t incount;
	size_t outcount;
	size_t round_seed;

	env_state() : mode(send_modes::specified), count(0), incount(0), outcount(0),
		round_seed(0) { }
};

class base_env
{
public:
	virtual int init() = 0;
	virtual e_start_info get_start_info() = 0;
	virtual int set_start_info(const n_start_info& inf) = 0;
	virtual e_send_info get() = 0;
	virtual int set(const n_send_info& inf) = 0;
	virtual int restart(const n_restart_info& inf) = 0;
	virtual verification_header get_header() const = 0;
	virtual e_restart_info get_restart_info() const = 0;
	virtual int stop() = 0;
	virtual int terminate() = 0;
	virtual env_state get_state() const = 0;

	base_env() { };

	virtual ~base_env() { };
};

