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
// #include "lisc.hpp"

using std::string;

#include <memory>
#include <list>
#include <typeindex>

struct Value;
typedef std::list<Value> List;

struct Value
{
    Value ()
        : type(std::type_index(typeid(void)))
    {}

    template<typename T>
    Value (T* v)
        : type(std::type_index(typeid(T))),
          atom(v)
    {}
    
    template<typename T>
    Value (const std::shared_ptr<T>& v)
        : type(std::type_index(typeid(T))),
          atom(v)
    {}
    
    Value (List& v)
        : type(std::type_index(typeid(void))),
          list(v)
    {}

    std::type_index type;
    std::shared_ptr<void> atom;
    List list;

    bool is_atom () const { return atom != nullptr; }
    bool is_list () const { return !is_atom(); }

    template<typename T>
    void set (const std::shared_ptr<T>& v)
    {
        type = std::type_index(typeid(T));
        atom = v;
    }

    template<typename T>
    T& get () const
    {
        if (type != std::type_index(typeid(T))) {
            throw std::logic_error("Value::get:Invalid type");
        }
         return *(T*)atom.get();
    }
};


template<typename T>
std::vector<std::shared_ptr<T>> get(const List& in)
{
    std::vector<std::shared_ptr<T>> out;
    for (auto& x : in) {
        if (x.type == std::type_index(typeid(T))) {
            out.push_back(std::shared_ptr<T>(std::static_pointer_cast<T>(x.atom)));
        }
    }
    return out;
}

template<typename T, unsigned int N>
std::vector<std::shared_ptr<T>> get(const List& in)
{
    std::vector<std::shared_ptr<T>> out;
    for (auto& x : in) {
        if (x.type == std::type_index(typeid(T))) {
            out.push_back(std::shared_ptr<T>(std::static_pointer_cast<T>(x.atom)));
        }
    }
    if (out.size() != N) {
        throw std::runtime_error("get: Bad number of items.");
    }
    return out;
}

template<typename T>
std::shared_ptr<T> get_one(const List& in)
{
    std::shared_ptr<T> out;
    for (auto& x : in) {
        if (x.type == std::type_index(typeid(T))) {
            if (out.get() == nullptr) {
                out = std::shared_ptr<T>(std::static_pointer_cast<T>(x.atom));
            }
            else {
                throw std::runtime_error("get_one: Multiple found");
            }
        }
    }
    if (out.get() == nullptr) {
        throw std::runtime_error("get_one: Item not found");
    }
    return out;
}

template<typename T>
std::shared_ptr<T> get_first(const List& in)
{
    for (auto& x : in) {
        if (x.type == std::type_index(typeid(T))) {
            return std::shared_ptr<T>(std::static_pointer_cast<T>(x.atom));
        }
    }
    return std::shared_ptr<T>(nullptr);
}


int main ()
{
    List l;
    l.push_back(Value(new double(0.8)));
    l.push_back(Value(std::make_shared<std::string>("Hello")));
    l.push_back(Value(new double(1.6)));
    l.push_back(Value(new std::string("World")));
    l.push_back(Value(new float(64.0f)));

    std::cout << "DOUBLE:\n";
    for (auto& x : get<double,2>(l)) {
        std::cout << *x << std::endl;
    }
    std::cout << "STRING:\n";
    for (auto& x : get<std::string>(l)) {
        std::cout << *x << std::endl;
    }
    std::cout << "FLOAT:\n";
    std::cout << *get_one<float>(l) << std::endl;
    std::cout << "INT?:\n";
    std::cout << (nullptr != get_first<int>(l)) << std::endl;

    // std::cout << v.get<double>() << std::endl;
    // std::cout << v.get<float>() << std::endl;
}









#if 0



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
			Rule("name", "[\\w+*-/=?.]+"),
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



Datum Evaluator::evaluate (const Datum& form)
{
	std::cout << "==[ EVAL ";
	print_list_item(form); std::cout << " -> ";

	// Number literal.
	// Evaluates to itself.
	if (form.is_number()) {
		return form;
	}

	// Symbol.
	if (form.is_name()) {
		const auto& it = bindings.find(form.name);
		if (it == bindings.end()) {
			throw std::runtime_error("No binding for "+form.name);
		}
		print_list_item(it->second); std::cout << std::endl;
		return it->second;

		// if (got_number(form.name)) {
		// 	return Datum(lookup_number(form));	
		// }
		// throw std::invalid_argument("Symbol "+form.name+" not defined.");
	}

	// Empty list.
	// Evaluates to itself.
	if (form.list.size() == 0) {
		print_list_item(form); std::cout << std::endl;
		return form;
	}

	// Non-empty list.
	// Considered a call.
	auto& temp = form.list;
	auto& head = temp.front();
	if (head.is_name()) {
		auto f = functions.find(head.name);
		if (f != functions.end()) {
			auto result = f->second(this, temp);
			print_list_item(result); std::cout << std::endl;
			return Datum(result);
		}
		throw std::invalid_argument("Cannot evaluate function "+head.name);
	}
	else {
		throw std::invalid_argument("Form head is not a symbol.");
	}

	print_list_item(Datum(temp)); std::cout << std::endl;
	return Datum(temp);
}


double Evaluator::lookup_number (const Datum& i)
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

bool Evaluator::got_number (const Datum& i)
{
	if (i.is_number()) return true;
	if (i.is_name()) {
		const auto& it = bindings.find(i.name);
		return it != bindings.end() && it->second.is_number();
	}
	// list
	return false;
}

void Evaluator::register_function(const std::string& name, function_t f)
{
	functions[name] = f;
}


struct core_functions
{
	static Datum comment(Evaluator* ev, const List& form)
	{
		return form;
	}

	static Datum plus(Evaluator* ev, const List& form)
	{
		double s = 0;
		for (size_t i = 1; i < form.size(); ++i) {
			s += ev->evaluate(form[i]).get_number();
		}
		return Datum(s);
	}

	static Datum minus(Evaluator* ev, const List& form)
	{
		double s = ev->evaluate(form[1]).get_number();
		for (size_t i = 2; i < form.size(); ++i) {
			s -= ev->evaluate(form[i]).get_number();
		}
		return Datum(s);
	}

	static Datum mul(Evaluator* ev, const List& form)
	{
		double s = 1;
		for (size_t i = 1; i < form.size(); ++i) {
			s *= ev->evaluate(form[i]).get_number();
		}
		return Datum(s);
	}

	static Datum div(Evaluator* ev, const List& form)
	{
		double s = ev->evaluate(form[1]).get_number();
		for (size_t i = 2; i < form.size(); ++i) {
			s /= ev->evaluate(form[i]).get_number();
		}
		return Datum(s);
	}
};

Evaluator::Evaluator ()
{
	bindings["pi"] = Datum(3.14159265358979323846);

	functions["bind"] = [](Evaluator* ev, const List& form)->Datum{
		if (form.size() != 3 ||	!form[1].is_name()) {
			throw std::runtime_error("bind needs a name and a value");
		}
		ev->bindings[form[1].name] = ev->evaluate(form[2]);
		return ev->bindings[form[1].name];
	};

	functions["vec3"] = [](Evaluator* ev, const List& form)->Datum{
		if (form.size() != 4) {
			throw std::runtime_error("vec3 must have 3 elements");
		}
		List r = { Datum("vec3") };
		for (int i = 1; i < 4; i++) {
			Datum it = ev->evaluate(form[i]);
			if (!it.is_number()) {
				throw std::runtime_error("vec3 elements must evaluate to numbers");
			}
			r.push_back(it);
		}
		return Datum(r);
	};

	functions["v+"] = [](Evaluator* ev, const List& form)->Datum{
		double a[3] = {0,0,0};
		for (size_t i = 1; i < form.size(); ++i) {
			if (!form[i].is_function("vec3")) throw std::runtime_error("add operands must be vec3's");

			for (int k = 0; k < 3; k++) {
				a[k] += ev->evaluate(form[i].list[k+1]).get_number();
			}
		}
		// List r = {  Datum("vec3"), Datum(a[0]),
		// 			Datum(a[1]), Datum(a[2]) };
		return Datum(makelist("vec3", a[0], a[1], a[2]));
	};

	register_function("--", core_functions::comment);
	register_function("+", core_functions::plus);
	register_function("-", core_functions::minus);
	register_function("*", core_functions::mul);
	register_function("/", core_functions::div);
}





Datum Evaluator::evaluate_file (const std::string& filename)
{
	std::ifstream ifs(filename);
	if (!ifs.is_open()) {
		std::cerr << "Error opening file '" << filename << "'!\n";
		return Datum(0);
	}
	std::string s;
	s.assign(std::istreambuf_iterator<char>(ifs),
			 std::istreambuf_iterator<char>());

	Scanner scanner(s);
	auto l = get_list(scanner);
	std::cout << "INPUT: ";
	print_list(l);

	List res;
	for (const auto& f : l) { 
		std::cout << std::endl << "FORM: ";
		print_list_item(f);
		std::cout << std::endl;
		auto l2 = evaluate(f);
		std::cout << "VALUE: ";
		print_list_item(l2);
		std::cout << std::endl;
		res.push_back(l2);
	}

	return Datum(res);
}

// void register_lisc_gray (Evaluator* ev);
// int main ()
// {
// 	try {
// 		Evaluator ev;
// 		register_lisc_gray(&ev);
// 		ev.evaluate_file("sample1.scene");
// 	}
// 	catch (std::exception& e) {
// 		std::cerr << "Exception caught: " << e.what() << std::endl;
// 		return 1;
// 	}

//   	return 0;
// }

#endif