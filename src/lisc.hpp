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
#include <sstream>

class LiscLogger {
public:
    LiscLogger ()
        : filename("<input>"), lineno(1)
    { }

    std::string filename;
    int lineno;
    std::stringstream ss;

    void set (const std::string& filename, int lineno)
    {
        this->filename = filename;
        this->lineno = lineno;
    }
    void set (int lineno)
    {
        this->lineno = lineno;
    }

    std::ostream& format ()
    {
        ss.str("");
        ss << filename << "::" << lineno << ": ";
        return ss;
    }

    std::string get() const
    {
        return ss.str();
    }
};

extern LiscLogger logger;

template<typename T>
inline
std::ostream& operator<< (const LiscLogger& l, const T& val)
{
    std::cout << l.filename << "::" << l.lineno << ": ";
    return std::cout;
}



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

    Value (const std::string& v)
        : type(std::type_index(typeid(std::string))),
          atom(new std::string(v))
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
    int lineno;
    std::string filename;

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
        if (is_list()) {
            std::stringstream ss;
            ss << filename << ":" << lineno << "::";
            ss << "While getting " << typeid(T).name() << " from " << *this << ": ";
            ss << "Is a list";
            throw std::runtime_error(ss.str());
        }
        if (type != std::type_index(typeid(T))) {
            std::stringstream ss;
            ss << filename << ":" << lineno << "::";
            ss << "While getting " << typeid(T).name() << " from " << *this << ": ";
            ss << "Wrong type";
            throw std::runtime_error(ss.str());
        }
         return *(T*)atom.get();
    }

    template<typename T>
    std::shared_ptr<T> get_ptr () const
    {
        if (is_list()) {
            std::stringstream ss;
            ss << filename << ":" << lineno << "::";
            ss << "While getting " << typeid(T).name() << " from " << *this << ": ";
            ss << "Is a list";
            throw std::runtime_error(ss.str());
        }
        if (type != std::type_index(typeid(T))) {
            std::stringstream ss;
            ss << filename << ":" << lineno << "::";
            ss << "While getting " << typeid(T).name() << " from " << *this << ": ";
            ss << "Wrong type";
            throw std::runtime_error(ss.str());
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
                logger.format()
                    << "While searching for attribute '" << name << "' in list " << in << ": "
                    << "Value list " << vallist << " length != 1";
                throw std::runtime_error(logger.get());
            }
            in.erase(it);
            return pop<T>(vallist);
        }
    }
    logger.format() << "While searching for attribute '" << name << "' in list " << in << ": "
            << "Not found";
    throw std::runtime_error(logger.get());
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

class Evaluator
{
public:
    Evaluator () {}

    void evaluate (Value& val);

    typedef std::function<bool(Value&, const std::string&, List&)> EvalSet;

    void add_set (EvalSet f)
    {
        funcs.push_back(f);
    }

private:
    std::vector<EvalSet> funcs;
};

Value parse_file (const char* filename);

#endif // _LISC_HPP_