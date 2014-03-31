#ifndef LISC_GRAY_H__
#define LISC_GRAY_H__

#include "gray.hpp"
#include "lisc.hpp"

bool evaluate_gray (Value& val, const std::string& name, List& args);

Transform pop_transforms (List& args);
 
#endif /* end of include guard: LISC_GRAY_H__ */