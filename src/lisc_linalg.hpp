#ifndef LISC_LINALG_H__
#define LISC_LINALG_H__

#include "lisc.hpp"
#include "Transform.hpp"

class LiscLinAlg : public ModuleBase
{
public:
	LiscLinAlg (Evaluator* ev);

	Datum vec3 (const List& form);
	Datum identity (const List& form);
	Datum translate (const List& form);
	Datum tmul (const List& form);

	std::vector<Transform> xforms;
	std::vector<glm::vec3> vec3s;
};

#endif /* end of include guard: LISC_LINALG_H__ */