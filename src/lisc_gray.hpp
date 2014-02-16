#ifndef LISC_GRAY_H__
#define LISC_GRAY_H__

#include "gray.hpp"
#include "lisc.hpp"

Scene* evaluate_scene (Value& description);

Value evaluate_xform (Value& val, List& args);
Value evaluate_prim (Value& val, List& args);
bool evaluate_immediates (Value& val, const std::string& name, List& args);
 
#endif /* end of include guard: LISC_GRAY_H__ */