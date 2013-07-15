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


class Scanner
{
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
		std::cout << "SCAN(" << backed << ")\n";
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
				std::cout << "SCAN " << std::string(o0, o1) << std::endl;
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
	Token tok;
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
	std::vector<Rule> rules = {
		Rule("lparen", "\\("),
		Rule("rparen", "\\)"),
		Rule("number", "-?(\\d*\\.)?\\d+"),
		Rule("name", "\\w+")
	};

	std::vector<Token> tokens;

	std::string s ("the subject of this 2.3 subsequence is -1 submarines");
	std::ifstream ifs ("sample1.scene");
	s.assign(std::istreambuf_iterator<char>(ifs),
			 std::istreambuf_iterator<char>());

	Scanner scanner(s);
	const List* l = get_list(scanner);
	print_list(l);

	// const char* o0;
	// const char* o1;
	// const char* p = s.c_str();
	// bool found = true;
	// while (found) {
	// 	found = false;

	// 	while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r') ++p;
	// 	// while (((*p) == ' ') || ((*p) == '\n') || ((*p) == '\t') || ((*p) == '\r')) {std::cout<<"skip\n";++p;}

	// 	for (const Rule& r : rules) {
	// 		if (trex_search(r.re.get(), p, &o0, &o1)) {
	// 			tokens.push_back(Token(r.type, std::string(o0, o1)));
	// 			std::cout << r.type << ": [";
	// 			for (const char* c = o0; c!=o1; ++c) std::cout << *c;
	// 			std::cout << "]" << std::endl;
	// 			p = o1;
	// 			found = true;
	// 			break;
	// 		}
	// 	}

	// }

	// std::cout << "----------------\n";
	// List root;
	// int nest = 0;
	// for (const auto& t : tokens) {
	// 	std::cout << t.type << std::endl;
	// 	switch (t.type) {
	// 		case Token::LPAREN:

	// }

  	return 0;
}

// struct Token
// {
// 	enum Type { STRING, NUMBER, LET, LPAREN, RPAREN, NEWLINE};
// 	Type	type;
// 	float	v_float;
// 	string	v_string;
// };

// class Lexer
// {
// 	// enum State {};
// 	// State state;
// 	vector<Token> tokens;
// 	// string buf;

// };


// void Lexer::feed ()
// {

// }


// void Lexer::feed (char s)
// {
// 	switch (state) {
// 		case S_IDLE:
// 			if (s == ' ' || s == '\t' || s == '\r' || s == '\n') next();
// 			else if (s == '(') { push(Token(Token::LPAREN)); next(); }
// 			else if (s == ')') { push(Token(Token::RPAREN)); next(); }
// 			else if (s == '=') { push(Token(Token::LET)); next(); }
// 			else if (s == '.' || s == '-' || (s >= '0' && s <= '9')) {
// 				state = S_NUMBER_START;
// 			}
// 			else if ((s >= 'a' && s <= 'z') || s == '_') {
// 				state = S_STRING_START;
// 			}
// 			;;;;
// 			break;
// 		case S_NUMBER_START:
// 			if (s == '-' || (s >= '0' && s <= '9')) {
// 				buf += s;
// 				state = S_NUMBER;
// 				next();
// 			}
// 			else if (s == '.') {
// 				buf += s;
// 				state = S_NUMBER_AFTER;
// 				next();
// 			}
// 			else error("Expecting starting number", s);
// 			break;
// 		case S_NUMBER:
// 			if (s >= '0' && s <= '9') {
// 				buf += s;
// 				next();
// 			}
// 			else if (s == '.') {
// 				buf += s;
// 				state = S_NUMBER_AFTER;
// 				next();
// 			}
// 			else {
// 				state = IDLE;
// 				push(Token(Token::NUMBER, std::stof(buf)));
// 				buf = "";
// 				next();
// 			}
// 			break;
// 		case S_NUMBER_AFTER:
// 			if (s >= '0' && s <= '9') {
// 				buf += s;
// 				next();
// 			}
// 			else {
// 				state = IDLE;
// 				push(Token(Token::NUMBER, std::stof(buf)));
// 				buf = "";
// 			}
// 			break;
// 		case S_STRING_START:
// 			if ((s >= 'a' && s <= 'z') || s == '_') {
// 				buf += s;
// 				state = S_STRING;
// 				next();
// 			}
// 			break;
// 		case S_STRING:
// 			if ((s >= 'a' && s <= 'z') || s == '_' || (s >= '0' && s <= '9')) {
// 				buf += s;
// 				next();
// 			}
// 			else {
// 				state = IDLE;
// 				push(Token(Token::STRING, buf));
// 				buf = "";
// 			}
// 			break;
// 	}
// }



