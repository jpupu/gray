#include "gray.hpp"
#include "lisc.hpp"

class Sphere : public Shape
{
public:
    bool intersect (Ray& r, Isect* isect);
};

Shape* make_sphere ()
{
    return new Sphere();
}

bool Sphere::intersect (Ray& ray, Isect* isect)
{
    const vec3& o = ray.o;
    const vec3& d = ray.d;

    float A = dot(d, d);
    float B = 2 * dot(d, o);
    float C = dot(o,o) - 1;

    float discrim = B*B - 4*A*C;
    if (discrim < 0) {
        return false;
    }

    // TODO: t0 and t1 can be written differently to improve precision.
    // see http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    float t0 = (-B - sqrtf(discrim)) / (2*A);
    float t1 = (-B + sqrtf(discrim)) / (2*A);
    if (t0 > t1) std::swap(t0, t1);

    float t = (t0 >= ray.tmin) ? t0 : t1;
    if (t < ray.tmin || t > ray.tmax) return false;

    ray.tmax = t;
    isect->p = o + t*d;
    isect->n = normalize(isect->p);

    return true;
}


class Plane : public Shape
{
public:
    bool intersect (Ray& ray, Isect* isect)
    {
        if (ray.d.y == 0) return false;

        float t = -ray.o.y / ray.d.y;
        
        if (t < ray.tmin || t > ray.tmax) return false;

        ray.tmax = t;
        isect->p = ray.o + t*ray.d;
        isect->n = vec3(0,1,0);

        return true;
    }
};

Shape* make_plane ()
{
    return new Plane();
}


class Rectangle : public Shape
{
public:
    bool intersect (Ray& ray, Isect* isect)
    {
        if (ray.d.y == 0) return false;

        float t = -ray.o.y / ray.d.y;

        if (t < ray.tmin || t > ray.tmax) return false;

        vec3 p = ray.o + t*ray.d;
        if (p.x < -1 || p.x > 1 || p.z < -1 || p.z > 1) return false;
        
        ray.tmax = t;
        isect->p = p;
        isect->n = vec3(0,1,0);

        return true;
    }
};

Shape* make_rectangle ()
{
    return new Rectangle();
}


std::vector<std::shared_ptr<Shape>> shape_pool;

Value evaluate_shape (Value& val, List& args)
{
    Shape* S;
    auto name = *pop<std::string>(args);
    if (name == "sphere") {
        S = new Sphere();
    }
    else if (name == "plane") {
        S = new Plane();
    }
    else if (name == "rectangle") {
        S = new Rectangle();
    }
    else {
        throw std::runtime_error("invalid shape name "+name);
    }

    std::shared_ptr<Shape> sh(S);
    shape_pool.push_back(sh);
    return Value({"_shape", sh});
}
