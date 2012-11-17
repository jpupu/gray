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
    bool intersect (const Ray& r, Isect* isect);
};

bool Sphere::intersect (const Ray& r, Isect* isect)
{
    vec3 d = r.d;
    vec3 o = r.o;

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

    float t = (t0 >= r.tmin) ? t0 : t1;
    if (t < r.tmin || t > r.tmax) return false;

    isect->t = t;
    isect->p = o + t*d;
    isect->n = normalize(isect->p);
    // probably should normalize p also to improve precision, or
    // not normalize n, as it should be good as-is. I dunno, maybe
    // this is fine.

    return true;
}













int main (int argc, char* argv[])
{
    Film film(256,256);

    Sphere sp;

    for (int yp = 0; yp < film.yres; yp++) {
        for (int xp = 0; xp < film.xres; xp++) {
            float xf = xp / (float)film.xres;
            float yf = yp / (float)film.yres;

            vec3 orig(0,0,3);
            vec3 dir(normalize(vec3(xf*2-1, yf*2-1, -1)));

            Ray ray(orig, dir);
            Isect isect;
            Spectrum L(0.0f);

            if (sp.intersect(ray, &isect)) {
                L = vec3(clamp(dot(isect.n, normalize(vec3(-1,1,1))), 0.0f, 1.0f));
            }

            film.add_sample(xf,yf, L);
        }
    }

    film.save("out.png");

    return 0;
}
