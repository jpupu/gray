#include "gray.hpp"
#include "lisc.hpp"


class PinholeCamera : public Camera
{
public:
    PinholeCamera ()
    {
        set_fov(90);
    }

    float film_d;

    void set_fov (float horizontal_fov_degrees)
    {
        film_d = (film_w/2) / tan(horizontal_fov_degrees/2 * M_PI/180);
    }


    virtual std::pair<vec3,vec3> get_vector (float x, float y, float u, float v) const
    {
        vec3 I(x, y, film_d);
        return std::make_pair(vec3(0,0,0), normalize(-I));
    }
    
};


class ThinLensCamera : public Camera
{
public:
    ThinLensCamera ()
    {
        set_focal_length(55);
        set_focus_distance(10);
        set_f_number(5.6);
    }

    void set_focal_length (float mm)
    {
        f = mm / 1000;
    }

    void set_focus_distance (float m)
    {
        d_o = -m;
    }

    void set_f_number (float N)
    {
        this->N = N;
    }

    // focal length
    // (represents optical power i.e. magnification)
    float f;
    // object distance (must be negative)
    // (represents focus distance)
    float d_o;
    // the f-number;
    float N;

    virtual std::pair<vec3,vec3> get_vector (float x, float y, float u, float v) const
    {
        // magnification
        float M = f / (f - d_o);
        // image distance
        float d_i = -M * d_o;
        // lens diameter
        float D = f / N;

        vec3 I(x, y, d_i);
        vec3 P(u*D/2, v*D/2, 0.0f);
        vec3 O(I.x/M, I.y/M, d_o);

        return std::make_pair(P, normalize(O - P));
    }

};


static
std::vector<std::shared_ptr<Camera>> cam_pool;

void evaluate_camera (Value& val, List& args)
{
    Camera* cam;
    auto name = *pop<std::string>(args);
    if (name == "pinhole") {
        auto* c = new PinholeCamera();
        c->set_fov(*pop_attr<double>("fov", std::make_shared<double>(60), args));
        cam = c;
    }
    else if (name == "thinlens") {
        auto* c = new ThinLensCamera();
        c->set_focal_length(*pop_attr<double>("focal_length", std::make_shared<double>(55), args));
        c->set_focus_distance(*pop_attr<double>("focus_distance", std::make_shared<double>(10), args));
        c->set_f_number(*pop_attr<double>("f_number", std::make_shared<double>(5.6), args));
        cam = c;
    }
    else {
        throw std::runtime_error("invalid camera name "+name);
    }
    cam->set_xform(*pop_attr<Transform>("_xform", std::make_shared<Transform>(), args));
    auto size = pop_attr<glm::vec2>("size", nullptr, args);
    if (size != nullptr) {
        cam->set_film(size->x, size->y);
    }

    std::shared_ptr<Camera> sh(cam);
    cam_pool.push_back(sh);
    val.reset({"_camera", sh});
}
