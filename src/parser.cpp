#include <string>
#include <iostream>
#include <vector>
#include <map>
extern "C" {
#include "trex/trex.h"
}
#include <memory>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>

using std::string;


class Scanner
{
public:
	struct Rule
	{
		Rule (const std::string& type, const std::string& pat)
			: type(type),
			pattern("^"+pat)
		{
			const char* err;
			re.reset(trex_compile(pattern.c_str(), &err), [](TRex* ptr){trex_free(ptr);});
			if (re == nullptr) {
				throw std::logic_error("malformed regex");
			}
		}

		std::string type;
		std::string pattern;
		std::shared_ptr<TRex> re;
	};

	struct Token
	{
		Token () {}
		Token (const std::string& t, const std::string& v)
			: type(t)
		{
			v_name = v;
			v_number = atof(v.c_str());
		}
		std::string type;
		float		v_number;
		std::string	v_name;
	};


private:
	std::string source;
	const char* p;
	std::vector<Rule> rules;
	Token current;
	bool backed;

public:
	Scanner (const std::string& source)
		: source(source),
		p(this->source.c_str()),
		backed(false)
	{
		rules = {
			Rule("lparen", "\\("),
			Rule("rparen", "\\)"),
			Rule("number", "-?(\\d*\\.)?\\d+"),
			Rule("name", "[\\w+-/=?.]+"),
			Rule("illegal", "[^ \\n\\t\\r]+")
		};
	}

	const Token& next ()
	{
		// std::cout << "SCAN(" << backed << ")\n";
		if (backed) {
			backed = false;
			return current;
		}

		while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r') ++p;

		const char* o0;
		const char* o1;
		for (const Rule& r : rules) {
			if (trex_search(r.re.get(), p, &o0, &o1)) {
				p = o1;
				current = Token(r.type, std::string(o0, o1));
				// std::cout << "SCAN " << std::string(o0, o1) << std::endl;
				return current;
			}
		}

		current =  Token("eot", "");
		return current;
	}

	const Token& back ()
	{
		if (backed) throw std::logic_error("backing backed");
		backed = true;
		return current;
	}
};




class Datum;
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

	bool is_function (const string& name) const
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

void print_list (const List& l, bool newline=true);

void print_list_item (const Datum& i)
{
	if (i.is_name()) std::cout << i.name << " ";
	else if (i.is_number()) std::cout << i.number << " ";
	else print_list(i.list, false);
}

void print_list (const List& l, bool newline)
{
	std::cout << "(";
	for (const Datum& i : l) print_list_item(i);
	std::cout << ") ";
	if (newline) std::cout << std::endl;
}

List get_list (Scanner& scanner)
{
	if (scanner.next().type != "lparen") {
		scanner.back();
		throw std::runtime_error("get_list expects lparen, got " + scanner.next().type);
	}

	List res;
	Scanner::Token tok;
	while ((tok = scanner.next()).type != "rparen") {
		if (tok.type == "eot") throw std::runtime_error("unexpected end of file");
		else if (tok.type == "number") {
			res.push_back(Datum(tok.v_number));
		}
		else if (tok.type == "name") {
			res.push_back(Datum(tok.v_name));
		}
		else if (tok.type == "lparen") {
			scanner.back();
			res.push_back(Datum(get_list(scanner)));
		}
		else if (tok.type == "illegal") {
			throw std::runtime_error("illegal token "+tok.v_name);
		}
		else throw std::runtime_error("bad token type "+tok.type);
	}

	return res;
}


class Evaluator
{
public:
	Datum evaluate (const Datum& form)
	{
		// Number literal.
		// Evaluates to itself.
		if (form.is_number()) {
			return form;
		}

		// Symbol.
		if (form.is_name()) {
			if (got_number(form.name)) {
				return Datum(lookup_number(form));	
			}
			throw std::invalid_argument("Symbol "+form.name+" not defined.");
		}

		// Empty list.
		// Evaluates to itself.
		if (form.list.size() == 0) {
			return form;
		}

		// Non-empty list.
		// Considered a call.
		auto& temp = form.list;
		auto& head = temp.front();
		if (head.is_name()) {
			auto f = functions.find(head.name);
			if (f != functions.end()) {
				return Datum(f->second(temp));
			}
			throw std::invalid_argument("Cannot evaluate function "+head.name);
		}
		else {
			throw std::invalid_argument("Form head is not a symbol.");
		}

		return Datum(temp);
	}


	double lookup_number (const Datum& i)
	{
		if (i.is_number()) return i.number;
		if (i.is_name()) {
			const auto& it = bindings.find(i.name);
			if (it == bindings.end()) {
				throw std::runtime_error("No binding for "+i.name);
			}
			if (!it->second.is_number()) {
				throw std::runtime_error(i.name + " is not a number");
			}
			return it->second.number;
		}
		throw std::invalid_argument("Can't cast list to number!");
	}

	bool got_number (const Datum& i)
	{
		if (i.is_number()) return true;
		if (i.is_name()) {
			const auto& it = bindings.find(i.name);
			return it != bindings.end() && it->second.is_number();
		}
		// list
		return false;
	}

	std::map<std::string, Datum> bindings =
	{
		{ "pi", Datum(3.14159265358979323846) }
	};

	std::map<std::string, std::function<Datum(const List&)>> functions =
	{
		{ "bind", [this](const List& form)->Datum{
			if (form.size() != 3 ||	!form[1].is_name()) {
				throw std::runtime_error("bind needs a name and a value");
			}
			this->bindings[form[1].name] = form[2];
			return form[2];
		}},
		{ "+", [this](const List& form)->Datum{
			double s = 0;
			for (size_t i = 1; i < form.size(); ++i) {
				s += this->evaluate(form[i]).get_number();
			}
			return Datum(s);
		}},
		{ "vec3", [this](const List& form)->Datum{
			if (form.size() != 4) {
				throw std::runtime_error("vec3 must have 3 elements");
			}
			List r = { Datum("vec3") };
			for (int i = 1; i < 4; i++) {
				Datum it = this->evaluate(form[i]);
				if (!it.is_number()) {
					throw std::runtime_error("vec3 elements must evaluate to numbers");
				}
				r.push_back(it);
			}
			return Datum(r);
		}},
		{ "v+", [this](const List& form)->Datum{
			double a[3] = {0,0,0};
			for (size_t i = 1; i < form.size(); ++i) {
				if (!form[i].is_function("vec3")) throw std::runtime_error("add operands must be vec3's");

				for (int k = 0; k < 3; k++) {
					a[k] += this->evaluate(form[i].list[k+1]).get_number();
				}
			}
			// List r = {  Datum("vec3"), Datum(a[0]),
			// 			Datum(a[1]), Datum(a[2]) };
			return Datum(makelist("vec3", a[0], a[1], a[2]));
		}}
	};
};




int main ()
{
	try {
		std::ifstream ifs ("sample1.scene");
		std::string s;
		s.assign(std::istreambuf_iterator<char>(ifs),
				 std::istreambuf_iterator<char>());

		Scanner scanner(s);
		auto l = get_list(scanner);
		std::cout << "INPUT: ";
		print_list(l);

		Evaluator e;
		for (const auto& f : l) { 
			std::cout << "FORM: ";
			print_list_item(f);
			std::cout << std::endl;
			auto l2 = e.evaluate(f);
			std::cout << "VALUE: ";
			print_list_item(l2);
			std::cout << std::endl;
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception caught: " << e.what() << std::endl;
		return 1;
	}

  	return 0;
}

