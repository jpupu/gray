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
#include "lisc.hpp"

LiscLogger logger;



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
        Token (const std::string& t, const std::string& v, int lineno=0)
            : type(t), lineno(lineno)
        {
            v_name = v;
            v_number = atof(v.c_str());
        }
        std::string type;
        float       v_number;
        std::string v_name;
        int lineno;
    };


private:
    std::string source;
    const char* p;
    std::vector<Rule> rules;
    Token current;
    bool backed;
    int lineno;
public:
    std::string filename;

public:
    Scanner (const std::string& source, const std::string& filename="<input>")
        : source(source),
        p(this->source.c_str()),
        backed(false),
        lineno(1),
        filename(filename)
    {
        rules = {
            Rule("comment-eol", "--.*\\n"),
            Rule("comment-limited", "{-.*-}"),
            Rule("lparen", "\\("),
            Rule("rparen", "\\)"),
            Rule("langle", "<"),
            Rule("rangle", ">"),
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

        while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r') {
            if (*p == '\n') lineno++;
            ++p;
        }

        const char* o0;
        const char* o1;
        for (const Rule& r : rules) {
            if (trex_search(r.re.get(), p, &o0, &o1)) {
                p = o1;
                current = Token(r.type, std::string(o0, o1), lineno);
                // std::cout << "SCAN " << std::string(o0, o1) << std::endl;
                return current;
            }
        }

        current =  Token("eot", "", lineno);
        return current;
    }

    const Token& back ()
    {
        if (backed) throw std::logic_error("backing backed");
        backed = true;
        return current;
    }
};




List get_list (Scanner& scanner, bool angle=false, bool enclosed=true)
{
    const std::string& opening = angle ? "langle" : "lparen";
    const std::string& closing = (enclosed ? (angle ? "rangle" : "rparen") : "eot");

    Scanner::Token first = scanner.next();
    if (enclosed) {
        if (first.type != opening) {
            scanner.back();
            throw std::runtime_error("get_list expects "+opening+", got " + scanner.next().type);
        }
    }
    else {
        scanner.back();
    }

    List res;
    Scanner::Token tok;
    while ((tok = scanner.next()).type != closing) {
        if (tok.type == "eot" && enclosed) {
            throw std::runtime_error("unexpected end of file");
        }
        else if (tok.type == "comment-eol" || tok.type == "comment-limited") {
            ;
        }
        else if (tok.type == "number") {
            res.push_back(tok.v_number);
            res.back().lineno = tok.lineno;
            res.back().filename = scanner.filename;
        }
        else if (tok.type == "name") {
            res.push_back(tok.v_name);
            res.back().lineno = tok.lineno;
            res.back().filename = scanner.filename;
        }
        else if (tok.type == "lparen") {
            scanner.back();
            res.push_back(get_list(scanner, false));
            res.back().lineno = tok.lineno;
            res.back().filename = scanner.filename;
        }
        else if (tok.type == "langle") {
            scanner.back();
            res.push_back(get_list(scanner, true));
            res.back().lineno = tok.lineno;
            res.back().filename = scanner.filename;
        }
        else if (tok.type == "illegal") {
            throw std::runtime_error("illegal token "+tok.v_name);
        }
        else throw std::runtime_error("bad token type "+tok.type);
    }

    if (angle) {
        res.push_front("vec"+std::to_string(res.size()));
        res.front().lineno = first.lineno;
        res.front().filename = scanner.filename;
    }

    return res;
}



Value parse_file (const char* filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in) {
        std::ostringstream contents;
        contents << in.rdbuf();
        in.close();

        Scanner scan(contents.str());
        return Value(get_list(scan, false, false));
    }
    throw std::runtime_error("error reading file");
}



void Evaluator::evaluate (Value& val, const std::string& prefix)
{
    // std::cout << prefix << "evaluate:: " << val << std::endl;
    logger.set(val.filename, val.lineno);
    if (val.is_list()) {
        List& l = val.list;
        if (l.size() == 0) return;
        // std::cout << prefix << "evaluate args:: " << std::endl;
        for (Value& v : l) {
            evaluate(v, prefix+"  ");
            // std::cout << prefix << "  :> " << v << std::endl;
        }

        if (l.front().is<std::string>()) {
            const std::string& name = l.front().get<std::string>();
            List args = tail(l);
            // std::cout << prefix << "evaluate function:: " << name << ":: " << args << std::endl;

            if (name == "def") {
                auto varname = *pop<std::string>(args);
                variables[varname] = args.front();
                // std::cout << prefix << "DEF:: " << varname << " := " << args.front() << std::endl;
                args.pop_front();
            }

            else if (variables.count(name) == 1) {
                val.reset(variables[name]);
            }

            else {
                for (auto f : funcs) {
                    if (f(val, name, args)) break;
                }
            }

            // assert_empty(args);
        }
    }
}



