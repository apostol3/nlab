#include <string>
#include <unordered_map>
#include <functional>
#include <rapidjson/document.h>

namespace jsonrpc
{
	namespace exceptions
	{
		class method_error : public std::exception
		{
		public:
			int get_code() const
			{
				return _code;
			}

			const std::string& get_message() const
			{
				return _message;
			}

			const rapidjson::Value& get_data() const
			{
				return _data;
			}
#ifdef WIN32
			const char* what() const override
#else
			const char* what() const noexcept override
#endif
			{
				return _message.c_str();
			}

			explicit method_error(const std::string& message, int code, rapidjson::Value&& data) :
				_code(code), _message(message), _data(std::move(data)) { }

			explicit method_error(const std::string& message, int code, rapidjson::Value& data) :
				_code(code), _message(message)
			{
				_data = data;
			}

			explicit method_error(const std::string& message, int code = 0) :
				_code(code), _message(message) { }

			explicit method_error(method_error&& rhs) :
				_code(rhs._code), _message(std::move(rhs._message)), _data(std::move(rhs._data)) { }

			method_error(method_error& rhs) :
				_code(rhs._code), _message(std::move(rhs._message))
			{
				_data = rhs._data;
			}

		protected:
			int _code;
			std::string _message;
			rapidjson::Value _data;
		};

		class invalid_parameters : public method_error
		{
		public:
			invalid_parameters() : method_error("Invalid parameters", -32602) { }

			explicit invalid_parameters(rapidjson::Value& data) : invalid_parameters()
			{
				_data = data;
			}
		};
	}

	class server
	{
	public:
		typedef std::function< rapidjson::Value(
			const rapidjson::Value& params, const rapidjson::Value& id,
			rapidjson::MemoryPoolAllocator< >& allocator
		) > rpc_method;
		std::string handle(const std::string& request);
		void add_method(const std::string& name, const rpc_method& method);
		void remove_method(const std::string& name);
		rpc_method& get_method(const std::string& name);
		const rpc_method& get_method(const std::string& name) const;
		bool has_method(const std::string& name) const;

		server() { }

		server(const server&) = delete;
		server& operator=(const server&) = delete;

	private:
		rapidjson::Value _call(const std::string& name, const rapidjson::Value& params,
		                       const rapidjson::Value& id,
		                       rapidjson::MemoryPoolAllocator< >& allocator);
		std::unordered_map< std::string, rpc_method > methods;
	};

	inline void server::add_method(const std::string& name, const rpc_method& method)
	{
		if (has_method(name))
		{
			get_method(name) = method;
		}
		methods.insert({name, method});
	}

	inline void server::remove_method(const std::string& name)
	{
		methods.erase(name);
	}

	inline server::rpc_method& server::get_method(const std::string& name)
	{
		return methods.at(name);
	}

	inline const server::rpc_method& server::get_method(const std::string& name) const
	{
		return methods.at(name);
	}

	inline bool server::has_method(const std::string& name) const
	{
		return (methods.count(name) > 0);
	}

	inline rapidjson::Value server::_call(const std::string& name, const rapidjson::Value& params,
	                                      const rapidjson::Value& id,
	                                      rapidjson::MemoryPoolAllocator< >& allocator)
	{
		return methods.at(name)(params, id, allocator);
	}

	inline std::string server::handle(const std::string& request)
	{
		static const std::string error_parse =
			{"{\"jsonrpc\": \"2.0\", \"error\": {\"code\": -32700, \"message\": \"Parse error\"},\"id\": null}"};
		static const std::string error_batch =
			{"{\"jsonrpc\": \"2.0\", \"error\": {\"code\": -32600, \"message\": \"Batch request not implemented yet\"}, \"id\": null}"};
		static const std::string error_request =
			{"{\"jsonrpc\": \"2.0\", \"error\" : {\"code\": -32600, \"message\" : \"Invalid Request\"}, \"id\" : null }"};
		static const std::string error_method =
			{",\"error\": {\"code\": -32601, \"message\": \"Method not found\"}"};
		static const std::string error_internal =
			{",\"error\": {\"code\": -32603, \"message\": \"Internal error\"}"};

		using namespace rapidjson;
		Document req;
		req.Parse(request.c_str());

		if (req.HasParseError())
		{
			return error_parse;
		}

		if (req.IsArray())
		{
			return error_batch;
		}

		if (!req.IsObject())
		{
			return error_request;
		}

		bool check_ver = req.HasMember("jsonrpc") &&
			req["jsonrpc"].IsString() &&
			!std::strcmp(req["jsonrpc"].GetString(), "2.0");

		if (!check_ver)
			return error_request;

		bool check_method = req.HasMember("method") && req["method"].IsString();
		if (!check_method)
			return error_request;

		bool check_id = !req.HasMember("id") ||
			(req["id"].IsNumber() || req["id"].IsString() || req["id"].IsNull());
		if (!check_id)
			return error_request;

		if (!req.HasMember("id") || req["id"].IsNull())
		{
			//request is just a notification, don't need to reply
			//don't need to check exists method or not
			try
			{
				_call(req["method"].GetString(),
				      req.HasMember("params") ? req["params"] : rapidjson::Value().SetNull(),
				      rapidjson::Value().Move(), req.GetAllocator());
			}
			catch (...) { }
			return "";
		}

		auto& req_id = req["id"];

		StringBuffer rep_str;
		Writer< StringBuffer > rep(rep_str);

		rep.StartObject();
		rep.Key("jsonrpc");
		rep.String("2.0");
		rep.Key("id");
		req_id.Accept(rep);

		if (!has_method(req["method"].GetString()))
		{
			for (auto c : error_method)
				rep_str.Put(c);
			rep.EndObject();
			rep_str.Put('\0');
			return rep_str.GetString();
		}


		try
		{
			auto ret = _call(req["method"].GetString(),
			                 req.HasMember("params") ? req["params"] : Value().SetNull(),
			                 req_id, req.GetAllocator());

			rep.Key("result");
			ret.Accept(rep);
		}
		catch (exceptions::method_error& e)
		{
			rep.Key("error");
			rep.StartObject();
			rep.Key("code");
			rep.Int(e.get_code());
			rep.Key("message");
			rep.String(e.get_message().c_str());
			if (e.get_data() != rapidjson::Value())
			{
				rep.Key("data");
				e.get_data().Accept(rep);
			}
			rep.EndObject();
		}
		catch (...)
		{
			for (auto c : error_internal)
				rep_str.Put(c);
		}

		rep.EndObject();
		rep_str.Put('\0');
		return rep_str.GetString();
	}
}

