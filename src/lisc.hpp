#ifndef _LISC_HPP_
#define _LISC_HPP_

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <algorithm>

#include <memory>
#include <list>
#include <typeindex>
#include <iostream>

struct Value;
typedef std::list<Value> List;



struct Value
{
    Value ()
        : type(std::type_index(typeid(void)))
    {}

    Value (std::initializer_list<Value> v)
        : type(std::type_index(typeid(void))),
          list(v)
    {}

    Value (double v)
        : type(std::type_index(typeid(double))),
          atom(new double(v))
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
    
    Value (const List& v)
        : type(std::type_index(typeid(void))),
          list(v)
    {}

    Value& reset (const Value& v)
    {
        type = v.type;
        atom = v.atom;
        list = v.list;
        return *this;
    }

    std::type_index type;
    std::shared_ptr<void> atom;
    List list;

    bool is_atom () const { return atom != nullptr; }
    bool is_list () const { return !is_atom(); }
    template<typename T>
    bool is () const { return type == std::type_index(typeid(T)); }

    template<typename T>
    void set (const std::shared_ptr<T>& v)
    {
        type = std::type_index(typeid(T));
        atom = v;
        list.clear();
    }

    template<typename T>
    T& get () const
    {
        if (type != std::type_index(typeid(T))) {
            throw std::logic_error("Value::get:Invalid type: got "+std::string(type.name())+", expecting "+std::type_index(typeid(T)).name());
        }
         return *(T*)atom.get();
    }

    template<typename T>
    std::shared_ptr<T> get_ptr () const
    {
        if (type != std::type_index(typeid(T))) {
            throw std::logic_error("Value::get_ptr:Invalid type: got "+std::string(type.name())+", expecting "+std::type_index(typeid(T)).name());
        }
         return std::shared_ptr<T>(std::static_pointer_cast<T>(atom));
    }

    friend std::ostream& operator<< (std::ostream& os, const Value& val) {
        if (val.is_atom()) {
            if (val.type == std::type_index(typeid(double))) {
                os << val.get<double>();
            }
            else if (val.type == std::type_index(typeid(std::string))) {
                os << '"' << val.get<std::string>() << '"';
            }
            else {
                os << '<' << val.type.name() << '>';
            }
        }
        else {
            // os << val.list;
            os << '(';
            for (const auto& x : val.list) {
                os << x << ' ';
            }
            os << ')';
        }
        return os;
    }
};

template<>
inline
Value::Value (const char* v)
    : type(std::type_index(typeid(std::string))),
      atom(new std::string(v))
{}


inline
std::ostream& operator<< (std::ostream& os, const List& list)
{
    os << '(';
    for (const auto& x : list) {
        os << x << ' ';
    }
    os << ')';
    return os;
}

template<typename T>
inline
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
inline
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
inline
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
inline
std::shared_ptr<T> get_first(const List& in)
{
    for (auto& x : in) {
        if (x.type == std::type_index(typeid(T))) {
            return std::shared_ptr<T>(std::static_pointer_cast<T>(x.atom));
        }
    }
    return std::shared_ptr<T>(nullptr);
}

inline
bool is_func(const std::string& name, const List& in)
{
    return in.front().is<std::string>() && (in.front().get<std::string>() == name);
}

template<typename T>
inline
bool all(const List& in)
{
    for (auto& x : in) {
        if (!x.is<T>()) return false;
    }
    return true;
}

template<typename T>
inline
void assert_all(const List& in)
{
    for (auto& x : in) {
        if (!x.is<T>()) {
            throw std::runtime_error("assert_all: Bad type");
        }
    }
}

inline
List tail (const List& in)
{
    return List(++in.begin(), in.end());
}

template<typename T>
inline
std::shared_ptr<T> pop (List& in)
{
    auto x = in.front().get_ptr<T>();
    in.pop_front();
    return x;
}

template<typename T>
inline
std::shared_ptr<T> pop_any (List& in)
{
    for (auto it = in.begin(); it != in.end(); ++it) {
        if (it->is<std::string>()) {
            auto x = it->get_ptr<T>();
            in.erase(it);
            return x;
        }
    }
    throw std::runtime_error("pop_any: none found");
}

inline
List pop_func (const char* name, List& in)
{
    for (auto it = in.begin(); it != in.end(); ++it) {
        if (it->is_list() && is_func(name, it->list)) {
            in.erase(it);
            return tail(it->list);
        }
    }
    std::cerr << "pop_func: in : " << in << std::endl;
    throw std::runtime_error("pop_func: none found");
}

template<typename T>
inline
std::shared_ptr<T> pop_attr (const char* name, List& in)
{
    for (auto it = in.begin(); it != in.end(); ++it) {
        if (it->is_list() && is_func(name, it->list)) {
            List vallist = tail(it->list);
            if (vallist.size() != 1) {
                std::cerr << "pop_attr: in : " << in << std::endl;
                std::cerr << "pop_attr: vallist : " << vallist << std::endl;
                throw std::runtime_error("pop_attr: bad list length");
            }
            in.erase(it);
            return pop<T>(vallist);
        }
    }
    std::cerr << "pop_attr: in : " << in << std::endl;
    throw std::runtime_error("pop_attr: none found");
}

template<typename T>
inline
std::shared_ptr<T> pop_attr (const char* name,
                             std::shared_ptr<T> default_value,
                             List& in)
{
    for (auto it = in.begin(); it != in.end(); ++it) {
        if (it->is_list() && is_func(name, it->list)) {
            List vallist = tail(it->list);
            if (vallist.size() != 1) {
                std::cerr << "pop_attr: in : " << in << std::endl;
                std::cerr << "pop_attr: vallist : " << vallist << std::endl;
                throw std::runtime_error("pop_attr: bad list length");
            }
            in.erase(it);
            return pop<T>(vallist);
        }
    }
    return default_value;
}

Value evaluate_material (Value& val, List& args);
Value evaluate_shape (Value& val, List& args);
Value evaluate_xform (Value& val, List& args);
Value evaluate_prim (Value& val, List& args);
bool evaluate_immediates (Value& val, const std::string& name, List& args);

class Evaluator
{
public:
    Evaluator () {}

    void evaluate (Value& val)
    {
    	std::cout << "evaluate:: " << val << std::endl;
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

                // assert_empty(args);
            }
        }

    }
};







#if 0
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

	std::string get_name () const
	{
		if (is_name()) return name;
		throw std::logic_error("Datum "+to_string()+" not a symbol");
	}

	List get_list () const
	{
		if (is_list()) return list;
		throw std::logic_error("Datum "+to_string()+" not a list");
	}

	operator double() const { return get_number(); }
	operator std::string() const { return get_name(); }
	operator List() const { return get_list(); }

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
#define REGISTER_METHOD_NAME(name, method) ev->register_function(name, [this](Evaluator*, const List& form)->Datum{return this->method(form);});
#endif

#endif // _LISC_HPP_