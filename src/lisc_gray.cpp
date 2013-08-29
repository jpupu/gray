#include "lisc_gray.hpp"
#include "shapes.hpp"
#include <iostream>


LiscGray::LiscGray (Evaluator* ev, LiscLinAlg* linalg)
	: ModuleBase(ev), linalg(linalg)
{
	counter=0;
	REGISTER_METHOD(prim);
	REGISTER_METHOD(sphere);
}

Datum LiscGray::prim (const List& form)
{
	List shape = eval(form.at(1));
	int shapenum = shape.at(1);
	Shape* s = shapes.at(shapenum);

	List xforml = eval(form.at(2));
	int xformnum = xforml.at(1);
	Transform& xform = linalg->xforms[xformnum];

	// Material* m = new Material { Spectrum(1.0f, 0.0f, .5f) };

	auto* p = new GeometricPrimitive();
	// p->mat = m;
	p->shape = s;
	p->world_from_prim = xform;

	primitives.push_back(p);

	return makelist("prim_", primitives.size()-1,0,0);
}

Datum LiscGray::sphere (const List& form)
{
	auto* s = make_sphere();
	shapes.push_back(s);
	return makelist("sphere_", shapes.size()-1);
}

