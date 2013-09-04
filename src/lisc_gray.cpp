#include "lisc_gray.hpp"
#include "shapes.hpp"
#include "materials.hpp"
#include <iostream>


LiscGray::LiscGray (Evaluator* ev)
	: ModuleBase(ev)
{ }

class LiscGrayImpl : public LiscGray
{
public:
	LiscLinAlg* linalg;


	LiscGrayImpl (Evaluator* ev, LiscLinAlg* linalg)
		: LiscGray(ev), linalg(linalg)
	{
		REGISTER_METHOD(prim);
		REGISTER_METHOD(sphere);
		REGISTER_METHOD(diffuse);
		REGISTER_METHOD(mirror);
		REGISTER_METHOD(plane);
		REGISTER_METHOD(rectangle);
	}

	Datum prim (const List& form)
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

	Datum sphere (const List& form)
	{
		auto* s = make_sphere();
		shapes.push_back(s);
		return makelist("shape_", shapes.size()-1);
	}

	Datum plane (const List& form)
	{
		auto* s = make_plane();
		shapes.push_back(s);
		return makelist("shape_", shapes.size()-1);
	}

	Datum rectangle (const List& form)
	{
		auto* s = make_rectangle();
		shapes.push_back(s);
		return makelist("shape_", shapes.size()-1);
	}

	Datum diffuse (const List& form)
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

	Datum mirror (const List& form)
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

};


LiscGray* LiscGray::create (Evaluator* ev, LiscLinAlg* linalg)
{
	return new LiscGrayImpl(ev, linalg);
}

