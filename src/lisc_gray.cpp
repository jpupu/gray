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




