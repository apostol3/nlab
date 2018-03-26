#pragma once

#include <windows.h>
#include <exception>
#include <iostream>

#include "remote_env.h"

class pipe_stream : public base_stream
{
	bool _connected;
	HANDLE _h_pipe;
	char _pipename[255];
	unsigned int _in_buf_size;
	unsigned int _out_buf_size;
	char* _buf;
public:
	pipe_stream(const char* pipename, unsigned in_buf_size, unsigned out_buf_size);
	pipe_stream(const pipe_stream&) = delete;
	pipe_stream(pipe_stream&& a);
	pipe_stream& operator =(const pipe_stream&) = delete;
	pipe_stream& operator =(pipe_stream&& a);
	~pipe_stream() override;
	void receive(void** ppd, size_t& sz) override;
	void send(const void* pd, size_t sz) override;
	bool is_connected() const override;
	void connect() override;
	void create() override;
	void disconnect() override;
	void close() override;
};

/* */
inline pipe_stream::pipe_stream(const char* pipename, unsigned in_buf_size, unsigned out_buf_size)
{
	_connected = false;
	_h_pipe = INVALID_HANDLE_VALUE;
	std::sprintf(_pipename, "\\\\.\\pipe\\%s", pipename);
	_in_buf_size = in_buf_size;
	_out_buf_size = out_buf_size;
	_buf = new char[in_buf_size]; //TODO: V121 http://www.viva64.com/en/V121 Implicit conversion 
	//of the type of 'new' operator's argument to size_t type.
}

/* */
inline pipe_stream::pipe_stream(pipe_stream&& a) : _connected(a._connected), _h_pipe(a._h_pipe),
                                                _in_buf_size(a._in_buf_size), _out_buf_size(a._out_buf_size),
                                                _buf(a._buf)
{
	strcpy_s(_pipename, 255, a._pipename);
	a._buf = nullptr;
}

/* */
inline pipe_stream& pipe_stream::operator=(pipe_stream&& a)
{
	std::swap(_h_pipe, a._h_pipe);
	_connected = a._connected;
	_in_buf_size = a._in_buf_size;
	_out_buf_size = a._out_buf_size;
	std::swap(_buf, a._buf);
	strcpy_s(_pipename, 255, a._pipename);
	return *this;
}

/* */
inline pipe_stream::~pipe_stream()
{
	delete[] _buf;
	if (_h_pipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_h_pipe);
	}
}

/* */
inline void pipe_stream::receive(void** ppd, size_t& sz)
{
	unsigned long int bytesRead = 0;
	char* buf = _buf;

	auto fSuccess = ReadFile(_h_pipe, // handle to pipe
	                         buf, // buffer to receive data
	                         _in_buf_size, // size of buffer
	                         &bytesRead, // number of bytes read
	                         nullptr); // not overlapped I/O
	if (!fSuccess || bytesRead == 0)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
		{
			throw std::exception("pipe_stream receive: client disconnected.");
		}

		throw std::exception("pipe_stream receive: ReadFile failed");
	}

	sz = bytesRead;
	*ppd = static_cast< void* >(buf);
}

/* */
inline void pipe_stream::send(const void* pd, size_t sz)
{
	if (sz > ULONG_MAX)
		throw std::invalid_argument("pipe send: cannot send to many bytes");

	unsigned long replyBytes = static_cast< unsigned long >(sz);
	unsigned long int written = 0;

	if (sz > _out_buf_size)
	{
		throw std::exception("Pipe buffer overflow");
	}

	auto fSuccess = WriteFile(_h_pipe, // handle to pipe
	                          pd, // buffer to write from
	                          replyBytes, // number of bytes to write
	                          &written, // number of bytes written
	                          nullptr); // not overlapped I/O
	if (!fSuccess || replyBytes != written)
	{
		throw std::exception("InstanceThread WriteFile failed");
	}
}

/* */
inline bool pipe_stream::is_connected() const
{
	return _connected;
}

/* */
inline void pipe_stream::connect()
{
	_h_pipe = CreateFileA(_pipename, // pipe name
	                   GENERIC_READ | // read and write access
	                   GENERIC_WRITE, 0, // no sharing
	                   nullptr, // default security attributes
	                   OPEN_EXISTING, // opens existing pipe
	                   0, // default attributes
	                   nullptr); // no template file

	// Break if the pipe handle isn't valid.
	if (_h_pipe == INVALID_HANDLE_VALUE)
	{
		throw std::exception("Could not open pipe");
	}

	DWORD dwMode = PIPE_READMODE_MESSAGE;
	auto fSuccess = SetNamedPipeHandleState(_h_pipe, // pipe handle
	                                        &dwMode, // new pipe mode
	                                        nullptr, // don't set maximum bytes
	                                        nullptr); // don't set maximum time
	if (!fSuccess)
	{
		throw std::exception("SetNamedPipeHandleState failed.");
	}

	_connected = true;
}

/* */
inline void pipe_stream::create()
{
	_h_pipe = CreateNamedPipeA(_pipename, // pipe name
	                        PIPE_ACCESS_DUPLEX, // read/write access
	                        PIPE_TYPE_MESSAGE | // message type pipe
	                        PIPE_READMODE_MESSAGE | // message-read mode
	                        PIPE_WAIT, // blocking mode
	                        PIPE_UNLIMITED_INSTANCES, // max. instances
	                        _out_buf_size, // output buffer size
	                        _in_buf_size, // input buffer size
	                        0, // client time-out
	                        nullptr); // default security attribute
	if (_h_pipe == INVALID_HANDLE_VALUE)
	{
		throw std::exception("CreateNamedPipe fail");
	}

	int res = ConnectNamedPipe(_h_pipe, nullptr);
	if (res == 0)
	{
		//TODO: format message
		throw std::exception("ConnectNamedPipe fail");
	}

	_connected = true;
}

/* */
inline void pipe_stream::disconnect()
{
	_connected = false;
	if (_h_pipe == INVALID_HANDLE_VALUE)
		return;


	CloseHandle(_h_pipe);
	_h_pipe = INVALID_HANDLE_VALUE;
}

/* */
inline void pipe_stream::close()
{
	_connected = false;
	if (_h_pipe == INVALID_HANDLE_VALUE)
		return;

	FlushFileBuffers(_h_pipe);
	DisconnectNamedPipe(_h_pipe);
	CloseHandle(_h_pipe);
	_h_pipe = nullptr;
}

