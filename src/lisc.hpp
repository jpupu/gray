#ifndef _LISC_HPP_
#define _LISC_HPP_

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

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
		throw std::logic_error("Datum not a number");
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


#endif // _LISC_HPP_