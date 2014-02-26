#include "gray.hpp"
#include "lisc.hpp"

class Sphere : public Shape
{
public:
    bool intersect (Ray& r, Isect* isect);
};


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


class Triangle : public Shape
{
public:
    vec3 v[3];

    bool intersect (Ray& ray, Isect* isect)
    {
        constexpr float EPSILON = 1e-6f;
        // Möller–Trumbore intersection algorithm
        // http://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
        // http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-9-ray-triangle-intersection/m-ller-trumbore-algorithm/

        vec3 e1 = v[1] - v[0];
        vec3 e2 = v[2] - v[0];
        vec3 pvec = cross(ray.d, e2);
        float det = dot(e1, pvec);

        // if the determinant is negative, the triangle is backfacing
        // if the determinant is close to zero, the ray won't hit
        if (det < EPSILON) return false;

        float inv_det = 1 / det;

        // Calculate u.
        vec3 tvec = ray.o - v[0];
        float u = dot(tvec, pvec) * inv_det;
        if (u < 0 || u > 1) return false;

        // Calculate v.
        vec3 qvec = cross(tvec, e1);
        float v = dot(ray.d, qvec) * inv_det;
        if (v < 0 || v > 1 || u + v > 1) return false;

        // Calculate t.
        float t = dot(e2, qvec) * inv_det;
        if (t < ray.tmin || t > ray.tmax) return false;

        // Hit.
        ray.tmax = t;
        isect->p = ray.o + t * ray.d;
        isect->n = normalize(cross(e1, e2));

        return true;
    }
};


static
std::vector<std::shared_ptr<Shape>> shape_pool;

void evaluate_shape (Value& val, List& args)
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
    else if (name == "triangle") {
        auto* t = new Triangle();
        t->v[0] = *pop<vec3>(args);
        t->v[1] = *pop<vec3>(args);
        t->v[2] = *pop<vec3>(args);
        S = t;
    }
    else {
        throw std::runtime_error("invalid shape name "+name);
    }

    std::shared_ptr<Shape> sh(S);
    shape_pool.push_back(sh);
    val.reset({"_shape", sh});
}
