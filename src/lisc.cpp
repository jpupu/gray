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
            res.push_back(get_list(scanner));
            res.back().lineno = tok.lineno;
            res.back().filename = scanner.filename;
        }
        else if (tok.type == "illegal") {
            throw std::runtime_error("illegal token "+tok.v_name);
        }
        else throw std::runtime_error("bad token type "+tok.type);
    }

    return res;
}



Value parse ()
{
    // const string source = "((prim (shape sphere) (material diffuse (R (rgb 1 1 1))) (xform (translate (vec3 0 0 -1))) (emit (rgb 1 0 0)) ))";
    const std::string source = "((prim\n(shape sphere)\n(material diffuse (R (rgb 1 1 1)))\n(xformd (translate (vec3 0 0 -1)))\n(emit (rgb 1 0 0))\n))";

    Scanner scan(source);

    return Value(get_list(scan));
}


#include "materials.hpp"
#include "shapes.hpp"
#include "lisc_gray.hpp"
#include "lisc_linalg.hpp"

void Evaluator::evaluate (Value& val)
{
    std::cout << "evaluate:: " << val << std::endl;
    logger.set(val.filename, val.lineno);
    if (val.is_list()) {
        List& l = val.list;
        if (l.size() == 0) return;
        for (Value& v : l) {
            evaluate(v);
        }

        if (l.front().is<std::string>()) {
            const std::string& name = l.front().get<std::string>();
            List args = tail(l);

            if (name == "+") {
                double sum = 0;
                while (!args.empty()) { sum += *pop<double>(args); }
                val.set(std::make_shared<double>(sum));
            }

            else if (name == "material") {
                val.reset( evaluate_material(val, args) );
            }
            
            else if (name == "shape") {
                val.reset( evaluate_shape(val, args) );
            }
            
            else if (name == "xform") {
                val.reset( evaluate_xform(val, args) );
            }
            
            else if (name == "prim") {
                val.reset( evaluate_prim(val, args) );
            }

            else if (evaluate_immediates(val, name, args)) {
                ;
            }

            else if (evaluate_linalg(val, name, args)) {
                ;
            }

            // assert_empty(args);
        }
    }
}



