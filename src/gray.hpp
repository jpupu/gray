#ifndef GRAY_HPP
#define GRAY_HPP

#include "mymath.hpp"
#include <vector>
#include "Transform.hpp"

typedef vec3 Spectrum;

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
        return true;
    }
};


#endif /* GRAY_HPP */
