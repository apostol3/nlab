#pragma once
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/encodedstream.h>
#include "tweann.h"


namespace json_routines
{
	inline nlab::tweann* load_from_string(const std::string& str)
	{
		using namespace rapidjson;
		std::string buf = str;
		nlab::tweann* nt = new nlab::tweann();
		StringStream s_stream(buf.c_str());
		GenericDocument< UTF16< > > doc;
		EncodedInputStream< UTF8< >, StringStream > eis(s_stream);
		doc.ParseStream< 0, UTF8< > >(eis);

		if (doc.HasParseError())
		{
			throw std::runtime_error("Load failed. JSON parse error");
		}

		nt->name = doc[L"name"].GetString();
		nt->note = doc[L"note"].GetString();
		nt->id = doc[L"id"].GetUint64();
		nt->fitness = doc[L"fitness"].GetDouble();

		unsigned maxnid = 0;
		unsigned maxlid = 0;

		auto& neurons = doc[L"neurons"];
		nt->nr.neurons.clear();
		for (auto dn = neurons.Begin(); dn != neurons.End(); dn++)
		{
			nt->nr.neurons.push_back(nlab::neuron());

			nlab::neuron* n = &nt->nr.neurons.back();
			n->c->id = (*dn)[L"id"].GetUint64();
			n->c->type = nlab::neuron_type((*dn)[L"type"].GetUint());
			n->c->e = (*dn)[L"energy"].GetDouble();
			n->c->ea = (*dn)[L"e_active"].GetDouble();

			n->c->in.clear();

			auto& in = (*dn)[L"in"];
			for (auto id = in.Begin(); id != in.End(); id++)
			{
				n->c->in.push_back(id->GetUint64());
			}

			n->c->out.clear();

			auto& out = (*dn)[L"out"];
			for (auto id = out.Begin(); id != out.End(); id++)
			{
				n->c->out.push_back(id->GetUint64());
			}

			n->rep = &nt->nr;
			if (maxnid < unsigned(n->c->id))
			{
				maxnid = n->c->id;
			}
		}

		auto& links = doc[L"links"];
		nt->lr.links.clear();
		for (auto dl = links.Begin(); dl != links.End(); dl++)
		{
			nt->lr.links.push_back(nlab::link());

			nlab::link* l = &nt->lr.links.back();
			l->id = (*dl)[L"id"].GetUint64();
			l->in_e = (*dl)[L"e_in"].GetDouble();
			l->out_e = (*dl)[L"e_out"].GetDouble();
			l->w = (*dl)[L"weight"].GetDouble();
			l->in = (*dl)[L"in"].GetUint64();
			l->out = (*dl)[L"out"].GetUint64();

			l->rep = &nt->lr;
			if (maxlid < unsigned(l->id))
			{
				maxlid = l->id;
			}
		}

		nt->nr.id_counter = maxnid + 1;
		nt->lr.id_counter = maxlid + 1;

		auto& visuals = doc[L"visual"];
		auto& v_neurons = visuals[L"neurons"];

		for (auto dv = v_neurons.Begin(); dv != v_neurons.End(); dv++)
		{
			int id = (*dv)[L"id"].GetUint64();
			nlab::neuron* n = nt->nr.get(id);
			if (n == nullptr)
			{
				continue;
			}

			n->v->x = (*dv)[L"x"].GetDouble();
			n->v->y = (*dv)[L"y"].GetDouble();
			n->v->r = (*dv)[L"r"].GetDouble();
		}
		return nt;
	}

	inline nlab::tweann* load_from_file(const std::string& file_name)
	{
		using namespace rapidjson;

		std::ifstream sv(file_name, std::ios_base::in);
		if (sv.fail())
		{
			throw std::runtime_error("Open failed. Couldn't open file");
		}

		StringBuffer buf;
		while (!sv.eof())
		{
			char a;
			sv.read(&a, 1);
			buf.Put(a);
		}

		buf.Pop(1);

		return load_from_string(buf.GetString());
	}

	inline std::string dump_to_string(const nlab::tweann* nt)
	{
		using namespace rapidjson;
		if (nt == nullptr)
		{
			throw std::runtime_error("Net pointer is null");
		}

		StringBuffer s;
		EncodedOutputStream< UTF8< >, StringBuffer > stream(s, false);
		PrettyWriter< EncodedOutputStream< UTF8< >, StringBuffer >, UTF16< >, UTF8< > > doc(stream);

		doc.StartObject();
		doc.Key(L"name");
		doc.String(nt->name.c_str());
		doc.Key(L"note");
		doc.String(nt->note.c_str());
		doc.Key(L"id");
		doc.Uint64(nt->id);
		doc.Key(L"fitness");
		doc.Double(nt->fitness);

		doc.Key(L"neurons");
		doc.StartArray();
		for (auto n = nt->nr.neurons.begin(); n != nt->nr.neurons.end(); ++n)
		{
			doc.StartObject();
			doc.Key(L"id");
			doc.Uint64(n->c->id); //TODO: V807 http://www.viva64.com/en/V807 Decreased performance.
								  //Consider creating a pointer to avoid using the 'n->c' expression repeatedly.
			doc.Key(L"type");
			doc.Uint(n->c->type);
			doc.Key(L"energy");
			doc.Double(n->c->e);
			doc.Key(L"e_active");
			doc.Double(n->c->ea);

			doc.Key(L"in");
			doc.StartArray();
			for (auto j = n->c->in.begin(); j != n->c->in.end(); ++j)
			{
				doc.Uint64(*j);
			}

			doc.EndArray();

			doc.Key(L"out");
			doc.StartArray();
			for (auto j = n->c->out.begin(); j != n->c->out.end(); ++j)
			{
				doc.Uint64(*j);
			}

			doc.EndArray();

			doc.EndObject();
		}

		doc.EndArray();

		doc.Key(L"links");
		doc.StartArray();
		for (auto l = nt->lr.links.begin(); l != nt->lr.links.end(); ++l)
		{
			doc.StartObject();
			doc.Key(L"id");
			doc.Uint64(l->id);
			doc.Key(L"e_in");
			doc.Double(l->in_e);
			doc.Key(L"e_out");
			doc.Double(l->out_e);
			doc.Key(L"weight");
			doc.Double(l->w);
			doc.Key(L"in");
			doc.Uint64(l->in);
			doc.Key(L"out");
			doc.Uint64(l->out);
			doc.EndObject();
		}

		doc.EndArray();

		doc.Key(L"visual");
		doc.StartObject();
		doc.Key(L"neurons");
		doc.StartArray();
		for (auto n = nt->nr.neurons.begin(); n != nt->nr.neurons.end(); ++n)
		{
			doc.StartObject();
			doc.Key(L"id");
			doc.Uint64(n->c->id);
			doc.Key(L"x");
			doc.Double(n->v->x); //TODO: V807 http://www.viva64.com/en/V807 Decreased performance.
								 //Consider creating a pointer to avoid using the 'n->v' expression repeatedly.
			doc.Key(L"y");
			doc.Double(n->v->y);
			doc.Key(L"r");
			doc.Double(n->v->r);
			doc.EndObject();
		}

		doc.EndArray();
		doc.EndObject();

		doc.EndObject();

		return s.GetString();
	}

	inline void dump_to_file(const nlab::tweann* nt, const std::string& file_name)
	{
		std::ofstream sv(file_name);
		if (sv.fail())
		{
			throw std::runtime_error("Save failed. Couldn't create file");
		}

		sv << dump_to_string(nt);
		sv.close();
	}
}
