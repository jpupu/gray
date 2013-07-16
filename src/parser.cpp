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
	ListItem () : list(new List()), name(nullptr) { }
	ListItem (const List* l) : list(l), name(nullptr) { }
	ListItem (const std::string& name) : list(nullptr), name(new std::string(name)) { }
	ListItem (double number) : list(nullptr), name(nullptr), number(number) { }

	const List* list;
	const std::string* name;
	double number;

	bool is_list () const { return list != nullptr; }
	bool is_name () const { return name != nullptr; }
	bool is_number () const { return !is_list() && !is_name(); }
	bool is_atom () const { return !is_list(); }
	bool is_atomic () const
	{
		return (is_atom() || 
				std::all_of(list->begin(), list->end(),
						    [](const ListItem& i){ return i.is_atom(); }));
	}
};




void print_list (const List* l, bool newline=true)
{
	std::cout << "(";
	for (const ListItem& i : *l) {
		if (i.is_name()) std::cout << *i.name << " ";
		else if (i.is_number()) std::cout << i.number << " ";
		else print_list(i.list, false);
	}
	std::cout << ") ";
	if (newline) std::cout << std::endl;
}

const List* get_list (Scanner& scanner)
{
	if (scanner.next().type != "lparen") {
		scanner.back();
		throw;
	}

	List* res = new List();
	Scanner::Token tok;
	while ((tok = scanner.next()).type != "rparen") {
		if (tok.type == "eot") throw;
		else if (tok.type == "number") {
			res->push_back(ListItem(tok.v_number));
		}
		else if (tok.type == "name") {
			res->push_back(ListItem(tok.v_name));
		}
		else if (tok.type == "lparen") {
			scanner.back();
			res->push_back(ListItem(get_list(scanner)));
		}
		else throw;
	}

	return res;
}



int main ()
{
	std::ifstream ifs ("sample1.scene");
	std::string s;
	s.assign(std::istreambuf_iterator<char>(ifs),
			 std::istreambuf_iterator<char>());

	Scanner scanner(s);
	const List* l = get_list(scanner);
	print_list(l);

  	return 0;
}

