#ifndef LISC_GRAY_H__
#define LISC_GRAY_H__

#include "lisc.hpp"
#include "gray.hpp"

class LiscGray : public ModuleBase
{
public:
	LiscGray (Evaluator* ev=nullptr);

	double counter;

	Datum prim (const List& form);

	Datum sphere (const List& form);

	std::vector<Primitive*> primitives;
	std::vector<Shape*> shapes;
};

 
#endif /* end of include guard: LISC_GRAY_H__ */