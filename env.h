#pragma once

#include <vector>

using env_task =  std::vector< double >;

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
	send_modes mode{send_modes::specified};
	size_t count{0};
	size_t incount{0};
	size_t outcount{0};
};

struct n_start_info
{
	size_t count{0};
	size_t round_seed{0};
};

struct n_send_info
{
	verification_header head{verification_header::fail};
	std::vector< env_task > data;
};

struct e_send_info
{
	verification_header head{verification_header::fail};
	std::vector< env_task > data;
};

struct n_restart_info
{
	size_t count{0};
	size_t round_seed{0};
};

struct e_restart_info
{
	std::vector< double > result{-1.0};
};

struct env_state
{
	send_modes mode{send_modes::specified};
	size_t count{0};
	size_t incount{0};
	size_t outcount{0};
	size_t round_seed{0};
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

	base_env() = default;

	virtual ~base_env() = default;
};

