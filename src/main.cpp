#include "Transform.hpp"
#include "film.hpp"

float frand ()
{
    return rand() / (float)RAND_MAX;
}

int min_elem (const vec3& a)
{
    return ((a.x < a.y)
            ? ((a.x < a.z)
               ? 0
               : (a.z < a.y) ? 2 : 1)
            : ((a.y < a.z)
               ? 1
               : (a.x < a.z) ? 0 : 2));
}

void orthonormal_basis (const vec3& r, vec3* s, vec3* t)
{
    int i = min_elem(r);
    int i2 = (i+1) % 3;
    int i3 = (i+2) % 3;
    if (i3 < i2) std::swap(i2,i3);

    (*s)[i] = 0;
    (*s)[i2] = -r[i3];
    (*s)[i3] = r[i2];
    (*s) = normalize(*s);

    (*t) = cross(r, *s);
}

/// Builds transform that changes vector from world space to tangent space.
Transform build_tangent_from_world (const vec3& normal)
{
    vec3 t, b;
    orthonormal_basis(normal, &t, &b);

    glm::mat4 m(glm::vec4(t, 1.0f),
                glm::vec4(b, 1.0f),
                glm::vec4(normal, 1.0f),
                glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    return Transform(glm::transpose(m), m);
}

/**
 * u_S = (c1,c2,c3) = vector in V
 * S = {v1,v2,v3} = basis of vector space V
 * u = c1*v1 + c2*v2 + c3*v3 = S^T * u_S
 */


vec3 sample_hemisphere (const vec2& uv)
{
    float z = uv[0];
    float r = sqrtf(1.0f - z*z);
    float phi = uv[1] * M_2PI;

    return vec3(cos(phi)*r, sin(phi)*r, z);
}


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

    Ray transform (const Transform& xform) const
    {
        return Ray(xform.point(o), xform.vector(d), tmin, tmax);
    }
};

class Material;

struct Isect
{
    vec3 p;
    vec3 n;
    Material* mat;
};


class Sphere
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

class Texture
{
public:
    Spectrum sample (const vec3& pos) const;

    Spectrum R;
};

class BSDF
{
public:
    virtual ~BSDF () {}

    /// @param wo [in]  exiting vector in tangent space, normalized
    /// @param wi [out] entering vector in tangent space, normalized
    /// @return reflectance f(wo,wi)
    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv) const = 0;
};

class Lambertian : public BSDF
{
public:
    Lambertian (const Spectrum& rho) : rho(rho) {}
    Spectrum rho;

    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv) const
    {
        *wi = sample_hemisphere(uv);
        return rho / (float)M_PI; // i still don't get why it's pi and not 2pi...
    }
};


class Material
{
public:
    Spectrum R;

    BSDF* get_bsdf (const vec3& p) const
    {
        return new Lambertian(R);
    }
};

class Primitive
{
public:
    virtual bool intersect (Ray& r, Isect* isect) const = 0;
};


class ListAggregate : public Primitive
{
public:
    std::vector<const Primitive*> prims;

    void add (const Primitive* p)
    {
        prims.push_back(p);
    }

    bool intersect (Ray& r, Isect* isect) const
    {
        bool hit = false;
        for (const Primitive* prim : prims) {
            hit |= prim->intersect(r, isect);
        }
        return hit;
    }
};

class GeometricPrimitive : public Primitive
{
public:
    Material* mat;
    Sphere* shape;
    Transform world_from_prim;
    mutable Transform prim_from_world;

    bool intersect (Ray& r, Isect* isect) const
    {
        prim_from_world = inverse(world_from_prim); // FIXME: dont do this every time!

        Ray ro = r.transform(prim_from_world);
        if (!shape->intersect(ro, isect)) {
            return false;
        }

        r.tmax = ro.tmax;
        isect->p = world_from_prim.point(isect->p);
        isect->n = world_from_prim.normal(isect->n);
        isect->mat = mat;
        return true;
    }
};






int main (int argc, char* argv[])
{
    Film film(256,256);

    Material mat1 = { Spectrum(1.0f, 0.0f, .5f) };
    Material mat2 = { Spectrum(.8f, 1.0f, .5f) };
    Sphere sp1;
    GeometricPrimitive prim1;
    prim1.mat = &mat1;
    prim1.shape = &sp1;
    prim1.world_from_prim = Transform::rotate(90, vec3(0,1,0));
    GeometricPrimitive prim2;
    prim2.mat = &mat2;
    prim2.shape = &sp1;
    prim2.world_from_prim = Transform::translate(vec3(1.4,1,-1));
    ListAggregate list;
    list.add(&prim1);
    list.add(&prim2);

    for (int yp = 0; yp < film.yres; yp++) {
        for (int xp = 0; xp < film.xres; xp++) {
            float xf = xp / (float)film.xres;
            float yf = yp / (float)film.yres;

            vec3 orig(0,0,3);
            vec3 dir(normalize(vec3(xf*2-1, yf*2-1, -1)));

            Ray ray(orig, dir);
            Isect isect;
            Spectrum L(0.0f);

            if (list.intersect(ray, &isect)) {
                BSDF* bsdf = isect.mat->get_bsdf(isect.p);
                Transform tangent_from_world = build_tangent_from_world(isect.n);
                vec3 wo_t = tangent_from_world.vector(-ray.d);
                vec3 wi_t;
                Spectrum f = bsdf->sample(wo_t, &wi_t, glm::vec2(frand(),frand()));
                delete bsdf;
                vec3 wo = inverse(tangent_from_world).vector(wi_t);
                L = f * (float)fmax(dot(wo, vec3(0,1,0)), 0.f);
            }

            film.add_sample(xf,yf, L);
        }
    }

    film.save("out.png");

    return 0;
}
