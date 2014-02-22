#include "lisc.hpp"
#include "Transform.hpp"

bool evaluate_linalg (Value& val, const std::string& name, List& args)
{
    if (name == "+") {
        double sum = 0;
        while (!args.empty()) {
            sum += *pop<double>(args);
        }
        val.reset(Value(sum));
        return true;
    }
    
    return false;
}

