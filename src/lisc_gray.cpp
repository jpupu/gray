#include "lisc_gray.hpp"
#include "shapes.hpp"
#include "materials.hpp"
#include <iostream>


LiscGray::LiscGray (Evaluator* ev, LiscLinAlg* linalg)
	: ModuleBase(ev), linalg(linalg)
{
	counter=0;
	REGISTER_METHOD(prim);
	REGISTER_METHOD(sphere);
	REGISTER_METHOD(diffuse);
	REGISTER_METHOD(mirror);
}

Datum LiscGray::prim (const List& form)
{
	List shape = eval(form.at(1));
	int shapenum = shape.at(1);
	Shape* s = shapes.at(shapenum);

	List mat = eval(form.at(2));
	int matnum = mat.at(1);
	Material* m = materials.at(matnum);

	List xforml = eval(form.at(3));
	int xformnum = xforml.at(1);
	Transform& xform = linalg->xforms[xformnum];

	auto* p = new GeometricPrimitive();
	p->mat = m;
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

Datum LiscGray::diffuse (const List& form)
{
	if (form.size() != 4) {
		throw std::runtime_error("diffuse must have 3 elements");
	}
	glm::vec3 v;
	for (int i = 1; i < 4; i++) {
		v[i-1] = eval(form.at(i));
	}

	auto* s = make_diffuse(v);
	materials.push_back(s);
	return makelist("material_", materials.size()-1);
}

Datum LiscGray::mirror (const List& form)
{
	if (form.size() != 4) {
		throw std::runtime_error("mirror must have 3 elements");
	}
	glm::vec3 v;
	for (int i = 1; i < 4; i++) {
		v[i-1] = eval(form.at(i));
	}

	auto* s = make_mirror(v);
	materials.push_back(s);
	return makelist("material_", materials.size()-1);
}

