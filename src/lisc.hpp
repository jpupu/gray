#ifndef _LISC_HPP_
#define _LISC_HPP_

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <algorithm>

struct Datum;
typedef std::vector<Datum> List;

struct Datum
{
	enum Type { LIST, NAME, NUMBER };

	Datum () : type(LIST) { }
	Datum (const List& l) : type(LIST), list(l) { }
	Datum (const std::string& name) : type(NAME), name(name) { }
	Datum (double number) : type(NUMBER), number(number) { }

	Type type;
	List list;
	std::string name;
	double number;

	double get_number () const
	{
		if (is_number()) return number;
		throw std::logic_error("Datum "+to_string()+" not a number");
	}

	bool is_list () const { return type == LIST; }
	bool is_name () const { return type == NAME; }
	bool is_number () const { return type == NUMBER; }
	bool is_atom () const { return type != LIST; }
	bool is_atomic () const
	{
		return (is_atom() || 
				std::all_of(list.begin(), list.end(),
						    [](const Datum& i){ return i.is_atom(); }));
	}

	bool is_function (const std::string& name) const
	{
		return is_list() && list[0].is_name() && list[0].name == name;
	}

	std::string to_string () const
	{
		switch (type) {
			case LIST: return "<list>";
			case NAME: return "<name \""+name+"\">";
			case NUMBER: return "<number ?>";//+std::to_string(number)+">";
		}
		char foop[32];
		sprintf(foop, "<invalid %#x>", type);
		return foop;
		// return "<invalid>";
	}

	// void match (const std::string& pattern) const
	// {
	// 	if (!is_list()) return false;
	// 	if (list.size() != pattern.size()) return false;
	// 	for (size_t i = 0; i < pattern.size(); i++) {
	// 		char c = pattern[i];
	// 		const Datum& d = list[i];
	// 		bool ok;
	// 		switch (c) {
	// 			case 'l': ok = d.is_list(); break;
	// 			case 's': ok = d.is_name(); break;
	// 			case 'n': ok = d.is_number(); break;
	// 		}
	// 		if (!ok) return false;
	// 	}
	// 	return true;
	// }

	// std::string stringrep () const
	// {
	// 	if (is_list()) {
	// 		std::string rep = "";
	// 		for (const Datum& d : list) {
	// 			switch (d.type) {
	// 				case LIST: rep += 'l'; break;
	// 				case NAME: rep += 's'; break;
	// 				case NUMBER: rep += 'n'; break;
	// 			}
	// 		}
	// 		return rep;
	// 	}
	// 	else return "";
	// }
};


template<typename T1>
List makelist (const T1& t1)
{
	List l = {Datum(t1)};
	return l;
}
template<typename T1, typename T2>
List makelist (const T1& t1, const T2& t2)
{
	List l = {Datum(t1),Datum(t2)};
	return l;
}
template<typename T1, typename T2, typename T3>
List makelist (const T1& t1, const T2& t2, const T3& t3)
{
	List l = {Datum(t1),Datum(t2),Datum(t3)};
	return l;
}
template<typename T1, typename T2, typename T3, typename T4>
List makelist (const T1& t1, const T2& t2, const T3& t3, const T4& t4)
{
	List l = {Datum(t1),Datum(t2),Datum(t3),Datum(t4)};
	return l;
}

class Evaluator
{
public:
	Evaluator ();

	Datum evaluate_file (const std::string& filename);

	Datum evaluate (const Datum& form);

	double lookup_number (const Datum& i);

	bool got_number (const Datum& i);

	std::map<std::string, Datum> bindings;

	typedef std::function<Datum(Evaluator* ev, const List& form)> function_t;

	void register_function(const std::string& name, function_t f);

	std::map<std::string, function_t> functions;
};

struct ModuleBase
{
	ModuleBase (Evaluator* ev=nullptr)
		: ev(ev)
	{ }

	Evaluator* ev;
	
	Datum eval (const Datum& d)
	{
		return ev->evaluate(d);
	}
};

#define REGISTER_METHOD(name) ev->register_function(#name, [this](Evaluator*, const List& form)->Datum{return this->name(form);});


#endif // _LISC_HPP_