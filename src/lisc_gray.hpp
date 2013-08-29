#ifndef LISC_GRAY_H__
#define LISC_GRAY_H__

#include "lisc.hpp"
#include "lisc_linalg.hpp"
#include "gray.hpp"

class LiscGray : public ModuleBase
{
public:
	LiscGray (Evaluator* ev, LiscLinAlg* linalg);

	double counter;

	Datum prim (const List& form);

	Datum sphere (const List& form);

	std::vector<Primitive*> primitives;
	std::vector<Shape*> shapes;
	LiscLinAlg* linalg;
};

 
#endif /* end of include guard: LISC_GRAY_H__ */