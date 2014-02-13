#include "lisc_gray.hpp"
// #include "shapes.hpp"
// #include "materials.hpp"
#include <iostream>

Scene* evaluate_scene (Value& description) {
    auto e = Evaluator();
    std::cout << description << std::endl;
    e.evaluate(description);
    std::cout << description << std::endl;

    Scene* scene = new Scene();
    ListAggregate* agg = new ListAggregate();
    std::shared_ptr<Primitive> p = nullptr;
    while ( (p = pop_attr<Primitive>("_prim", nullptr, description.list)) ) {
        agg->add(p.get());
    }
    scene->primitives = agg;
    return scene;
}

Value evaluate_xform (Value& val, List& args)
{
    Transform T;
    std::cout << "evalxform " << args << std::endl;

    for (auto factor : args) {
        std::cout << "evalxform factor " << factor << std::endl;
        if (!factor.is_list()) throw std::runtime_error("evaluate_xform: item not a list");

        auto aa = factor.list;
        auto name = *pop<std::string>(aa);
        if (name == "translate") {
            auto v = *pop<glm::vec3>(aa);
            T = T * Transform::translate(v);
        }
        else if (name == "scale") {
            auto v = *pop<double>(aa);
            T = T * Transform::scale(glm::vec3(v));
        }
        else if (name == "rotate") {
            auto angle = *pop<double>(aa);
            auto axis = *pop<glm::vec3>(aa);
            T = T * Transform::rotate(angle, axis);
        }
        else {
            throw std::runtime_error("invalid transform name");
        }
    }

    return Value({"_xform", new Transform(T)});
}


std::vector<std::shared_ptr<Primitive>> primitive_pool;

Value evaluate_prim (Value& val, List& args)
{
    auto* p = new GeometricPrimitive();
    p->mat = pop_attr<Material>("_material", args).get();
    p->shape = pop_attr<Shape>("_shape", args).get();
    p->world_from_prim = *pop_attr<Transform>("_xform", args);
    p->Le = *pop_attr<Spectrum>("emit", std::shared_ptr<Spectrum>(new Spectrum(0)), args);

    std::shared_ptr<Primitive> sh(dynamic_cast<Primitive*>(p));
    primitive_pool.push_back(sh);
    return Value({"_prim", sh});
}

bool evaluate_immediates (Value& val, const std::string& name, List& args)
{
    if (name == "rgb") {
        float r = *pop<double>(args);
        if (args.empty()) {
            val.set(std::make_shared<Spectrum>(r));
        }
        else {
            float g = *pop<double>(args);
            float b = *pop<double>(args);
            val.set(std::make_shared<Spectrum>(r,g,b));
        }
        return true;
    }
    
    else if (name == "vec3") {
        float r = *pop<double>(args);
        if (args.empty()) {
            val.set(std::make_shared<glm::vec3>(r));
        }
        else {
            float g = *pop<double>(args);
            float b = *pop<double>(args);
            val.set(std::make_shared<glm::vec3>(r,g,b));
        }
        return true;
    }
    
    return false;
}









#if 0
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
		REGISTER_METHOD(lightprim);
		REGISTER_METHOD(sphere);
		REGISTER_METHOD(diffuse);
		REGISTER_METHOD(mirror);
		REGISTER_METHOD(glass);
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
		p->Le = Spectrum(0);

		primitives.push_back(p);

		return makelist("prim_", primitives.size()-1,0,0);
	}
	Datum lightprim (const List& form)
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
		p->Le = Spectrum(100.0f);

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

	Datum glass (const List& form)
	{
		if (form.size() != 4) {
			throw std::runtime_error("glass must have 3 elements");
		}
		glm::vec3 v;
		for (int i = 1; i < 4; i++) {
			v[i-1] = eval(form.at(i));
		}

		auto* s = make_glass(v);
		materials.push_back(s);
		return makelist("material_", materials.size()-1);
	}

};


LiscGray* LiscGray::create (Evaluator* ev, LiscLinAlg* linalg)
{
	return new LiscGrayImpl(ev, linalg);
}

#endif
