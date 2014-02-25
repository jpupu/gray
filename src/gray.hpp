#ifndef GRAY_HPP
#define GRAY_HPP

#include "mymath.hpp"
#include <vector>
#include "Transform.hpp"

typedef vec3 Spectrum;

struct Ray
{
    static constexpr float epsilon = 1e-5f;

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
    Spectrum Le; // this is oversimplified
};


class Shape
{
public:
	virtual ~Shape () {}
    virtual bool intersect (Ray& r, Isect* isect) = 0;
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


class Material
{
public:
    virtual ~Material () {}
    virtual BSDF* get_bsdf (const vec3& p) const = 0;
};

class Primitive
{
public:
    virtual ~Primitive () {}
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
    Shape* shape;
    Transform world_from_prim;
    mutable Transform prim_from_world;
    Spectrum Le;

    bool intersect (Ray& r, Isect* isect) const
    {
        prim_from_world = inverse(world_from_prim); // FIXME: dont do this every time!

        Ray ro = r.transform(prim_from_world);
        if (!shape->intersect(ro, isect)) {
            return false;
        }

        r.tmax = ro.tmax;
        isect->p = world_from_prim.point(isect->p);
        isect->n = normalize(world_from_prim.normal(isect->n));
        isect->mat = mat;
        isect->Le = Le;
        return true;
    }
};


class Camera
{
public:
    Camera ()
    {
        set_film(36, 24); // standard full-frame 35mm.
    }

    void set_xform (const Transform& w_from_c)
    {
        world_from_cam = w_from_c;
    }

    void set_film (float w_mm, float h_mm)
    {
        film_w = w_mm / 1000.0f;
        film_h = h_mm / 1000.0f;
    }

    Transform world_from_cam;
    float film_w;
    float film_h;

    // x,y,u,v = 0..1
    Ray generate_ray (float x, float y, float u, float v) const
    {
        auto d = get_vector((x*2-1)*film_w/2,
                            (y*2-1)*film_h/2,
                            (u*2-1),
                            (v*2-1));
        return Ray(d.first, d.second).transform(world_from_cam);
    }

protected:
    virtual std::pair<vec3,vec3> get_vector (float x, float y, float u, float v) const = 0;
};

class Scene
{
public:
    Primitive* primitives;
    Camera* camera;

    bool intersect (Ray& ray, Isect* isect) const
    {
        return primitives->intersect(ray, isect);
    }
};


#endif /* GRAY_HPP */
