#include "Transform.hpp"
#include "film.hpp"


struct Ray
{
    static constexpr float epsilon = 1e-6f;

    vec3 o;
    vec3 d;
    float tmin, tmax;

    Ray (const vec3& o, const vec3& d,
         float tmin = epsilon, float tmax = INFINITY)
        : o(o), d(d), tmin(tmin), tmax(tmax)
    { }

};

struct Isect
{
    float t;
    vec3 p;
    vec3 n;
};


class Sphere
{
public:
    bool intersect (const Ray& r, Isect* isect, const Transform& world_from_object);
};

bool Sphere::intersect (const Ray& ray, Isect* isect, const Transform& world_from_object)
{
    vec3 o = inverse(world_from_object).point(ray.o);
    vec3 d = inverse(world_from_object).vector(ray.d);

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

    isect->t = t;
    isect->p = ray.o + t*ray.d;
    isect->n = world_from_object.normal(normalize(o+t*d));

    return true;
}

class Material
{
public:
    Spectrum R;
};

class Surface
{
public:
    Material* mat;
    Sphere* shape;
    Transform world_from_object;

    bool intersect (const Ray& r, Isect* isect)
    {
        return shape->intersect(r, isect, world_from_object);
    }
};










int main (int argc, char* argv[])
{
    Film film(256,256);

    Material mat1 = { Spectrum(1.0f, 0.0f, .5f) };
    Sphere sp1;
    Surface surf1 = { &mat1, &sp1, Transform::rotate(90, vec3(0,1,0)) };

    for (int yp = 0; yp < film.yres; yp++) {
        for (int xp = 0; xp < film.xres; xp++) {
            float xf = xp / (float)film.xres;
            float yf = yp / (float)film.yres;

            vec3 orig(0,0,3);
            vec3 dir(normalize(vec3(xf*2-1, yf*2-1, -1)));

            Ray ray(orig, dir);
            Isect isect;
            Spectrum L(0.0f);

            if (surf1.intersect(ray, &isect)) {
                float f = clamp(dot(isect.n, normalize(vec3(-1,1,1))), 0.0f, 1.0f);
                L = surf1.mat->R * f;
            }

            film.add_sample(xf,yf, L);
        }
    }

    film.save("out.png");

    return 0;
}
