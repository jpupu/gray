#include "gray.hpp"

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
        isect->n = vec3(0,0,1);

        return true;
    }
};

Shape* make_plane ()
{
    return new Plane();
}
