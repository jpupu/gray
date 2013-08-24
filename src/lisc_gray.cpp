#include "lisc_gray.hpp"
#include "shapes.hpp"
#include <iostream>


LiscGray::LiscGray (Evaluator* ev)
	: ModuleBase(ev)
{
	counter=0;
	REGISTER_METHOD(prim);
	REGISTER_METHOD(sphere);
}

Datum LiscGray::prim (const List& form)
{
	List shape = eval(form.at(1));
	int shapenum = shape.at(1);
	Shape* s = shapes[shapenum];

	Material* m = new Material { Spectrum(1.0f, 0.0f, .5f) };

	auto* p = new GeometricPrimitive();
	p->mat = m;
	p->shape = s;

	primitives.push_back(p);

	return makelist("prim_", primitives.size()-1,0,0);
}

Datum LiscGray::sphere (const List& form)
{
	auto* s = make_sphere();
	shapes.push_back(s);
	return makelist("sphere_", shapes.size()-1);
}

