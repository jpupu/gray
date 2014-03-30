#include "lisc_gray.hpp"
// #include "shapes.hpp"
// #include "materials.hpp"
#include <iostream>
#include "lisc_linalg.hpp"

void evaluate_shape (Value& val, List& args);
void evaluate_texture (Value& val, List& args);
void evaluate_material (Value& val, List& args);
void evaluate_camera (Value& val, List& args);
void evaluate_skylight (Value& val, List& args);



void evaluate_xform (Value& val, List& args)
{
    Transform T;
    // std::cout << "evalxform " << args << std::endl;

    for (auto factor : args) {
        // std::cout << "evalxform factor " << factor << std::endl;
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
    
    val.reset({"_xform", new Transform(T)});
}


void evaluate_prim (Value& val, List& args)
{
    auto* p = new GeometricPrimitive();
    p->mat = pop_attr<Material>("_material", args);
    p->shape = pop_attr<Shape>("_shape", args);
    p->world_from_prim = *pop_attr<Transform>("_xform", args);
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
    else if (name == "prim") {
        evaluate_prim(val, args);
        return true;    
    }
    else if (name == "xform") {
        evaluate_xform(val, args);
        return true;    
    }
    else if (name == "material") {
        evaluate_material(val, args);
        return true;    
    }
    else if (name == "texture") {
        evaluate_texture(val, args);
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




