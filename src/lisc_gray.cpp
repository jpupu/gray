#include "lisc_gray.hpp"
// #include "shapes.hpp"
// #include "materials.hpp"
#include <iostream>
#include "lisc_linalg.hpp"

void evaluate_shape (Value& val, List& args);
bool evaluate_texture (Value& val, const std::string& name, List& args);
bool evaluate_material (Value& val, const std::string& name, List& args);
void evaluate_camera (Value& val, List& args);
void evaluate_skylight (Value& val, List& args);


bool evaluate_transform (Value& val, const std::string& name, List& args)
{
    Transform T;
    if (name == "ident") {
        ; // nop
    }
    else if (name == "translate") {
        auto v = *pop<vec3>(args);
        T = Transform::translate(v);
    }
    else if (name == "rotate") {
        auto a = *pop<double>(args);
        auto v = *pop<vec3>(args);
        T = Transform::rotate(a, v);
    }
    else if (name == "scale") {
        auto a = *pop<double>(args);
        // auto v = *pop<vec3>(args);
        T = Transform::scale(vec3(a, a, a));
    }
    else {
        return false;
    }

    val.reset(new Transform(T));
    return true;    
}


void evaluate_xform (Value& val, List& args)
{
    Transform T;
    // std::cout << "evalxform " << args << std::endl;

    assert_all<Transform>(args);

    for (auto t : args) {
        T = T * t.get<Transform>();
    }
    
    val.reset({"_xform", new Transform(T)});
}

Transform pop_transforms (List& args)
{
    Transform T = Transform();
    for (auto& t : pop_all<Transform>(args)) {
        T = T * (*t);
    }
    return T;
}

void evaluate_prim (Value& val, List& args)
{
    auto* p = new GeometricPrimitive();
    p->mat = pop_any<Material>(args);
    p->shape = pop_any<Shape>(args);

    p->world_from_prim = pop_transforms(args);

    p->Le = *pop_attr<Spectrum>("emit", std::shared_ptr<Spectrum>(new Spectrum(0)), args);

    std::shared_ptr<Primitive> sh(dynamic_cast<Primitive*>(p));
    val.reset({"_prim", sh});
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
    
    else if (name == "vec4") {
        float r = *pop<double>(args);
        if (args.empty()) {
            val.set(std::make_shared<glm::vec4>(r));
        }
        else {
            float g = *pop<double>(args);
            float b = *pop<double>(args);
            float a = *pop<double>(args);
            val.set(std::make_shared<glm::vec4>(r,g,b,a));
        }
        return true;
    }
    
    else if (name == "vec2") {
        float r = *pop<double>(args);
        if (args.empty()) {
            val.set(std::make_shared<glm::vec2>(r));
        }
        else {
            float g = *pop<double>(args);
            val.set(std::make_shared<glm::vec2>(r,g));
        }
        return true;
    }
    
    return false;
}

bool evaluate_gray (Value& val, const std::string& name, List& args)
{
    if (evaluate_immediates(val, name, args)) return true;
    if (evaluate_transform(val, name, args)) return true;
    if (evaluate_material(val, name, args)) return true;
    if (evaluate_texture(val, name, args)) return true;
    else if (name == "prim") {
        evaluate_prim(val, args);
        return true;    
    }
    else if (name == "xform") {
        evaluate_xform(val, args);
        return true;    
    }
    else if (name == "shape") {
        evaluate_shape(val, args);
        return true;    
    }
    else if (name == "camera") {
        evaluate_camera(val, args);
        return true;    
    }
    else if (name == "skylight") {
        evaluate_skylight(val, args);
        return true;    
    }

    return false;
}




