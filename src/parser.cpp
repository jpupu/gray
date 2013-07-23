#include <string>
#include <iostream>
#include <vector>
extern "C" {
#include "trex/trex.h"
}
#include <memory>
#include <fstream>
#include <cstdlib>
#include <algorithm>

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
				throw;
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
			Rule("name", "\\w+")
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
		if (backed) throw;
		backed = true;
		return current;
	}
};




class ListItem;
typedef std::vector<ListItem> List;

struct ListItem
{
	enum Type { LIST, NAME, NUMBER };

	ListItem () : type(LIST) { }
	ListItem (const List& l) : type(LIST), list(l) { }
	ListItem (const std::string& name) : type(NAME), name(name) { }
	ListItem (double number) : type(NUMBER), number(number) { }

	Type type;
	List list;
	std::string name;
	double number;

	bool is_list () const { return type == LIST; }
	bool is_name () const { return type == NAME; }
	bool is_number () const { return type == NUMBER; }
	bool is_atom () const { return type != LIST; }
	bool is_atomic () const
	{
		return (is_atom() || 
				std::all_of(list.begin(), list.end(),
						    [](const ListItem& i){ return i.is_atom(); }));
	}
};



void print_list (const List& l, bool newline=true);

void print_list_item (const ListItem& i)
{
	if (i.is_name()) std::cout << i.name << " ";
	else if (i.is_number()) std::cout << i.number << " ";
	else print_list(i.list, false);
}

void print_list (const List& l, bool newline)
{
	std::cout << "(";
	for (const ListItem& i : l) print_list_item(i);
	std::cout << ") ";
	if (newline) std::cout << std::endl;
}

List get_list (Scanner& scanner)
{
	if (scanner.next().type != "lparen") {
		scanner.back();
		throw;
	}

	List res;
	Scanner::Token tok;
	while ((tok = scanner.next()).type != "rparen") {
		if (tok.type == "eot") throw;
		else if (tok.type == "number") {
			res.push_back(ListItem(tok.v_number));
		}
		else if (tok.type == "name") {
			res.push_back(ListItem(tok.v_name));
		}
		else if (tok.type == "lparen") {
			scanner.back();
			res.push_back(ListItem(get_list(scanner)));
		}
		else throw;
	}

	return res;
}


class Evaluator
{
public:
	ListItem evaluate (const ListItem& item)
	{
		if (item.is_number()) {
			return item;
		}
		if (item.is_name()) {
			// TODO: var lookup
			return ListItem(item.name);
		}

		// evaluate sublists
		List temp;
		for (auto& x : item.list) {
			temp.push_back(evaluate(x));
		}

		// evaluate function
		auto& head = temp.front();
		if (head.is_name()) {
			if (head.name == "sum") {
				double s = 0;
				for (size_t i = 1; i < temp.size(); ++i) {
					s += temp[i].number;
				}
				return ListItem(s);
			}
			if (head.name == "prod") {
				double s = 1;
				for (size_t i = 1; i < temp.size(); ++i) {
					s *= temp[i].number;
				}
				return ListItem(s);
			}
		}

		return ListItem(temp);
	}
};




int main ()
{
	std::ifstream ifs ("sample1.scene");
	std::string s;
	s.assign(std::istreambuf_iterator<char>(ifs),
			 std::istreambuf_iterator<char>());

	Scanner scanner(s);
	auto l = get_list(scanner);
	print_list(l);

	Evaluator e;
	auto l2 = e.evaluate(ListItem(l));
	print_list_item(l2);


  	return 0;
}

