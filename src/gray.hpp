#ifndef GRAY_HPP
#define GRAY_HPP

#include <memory>
using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
#include "mymath.hpp"
#include <vector>
#include "Transform.hpp"
#include "random.hpp"

typedef vec3 Spectrum;

#include <iostream>
namespace debug {
    extern int x,y,sample,nest,on;

    inline
    void set (int x, int y, int sample)
    {
#if ENABLE_DEBUG
        debug::x = x;
        debug::y = y;
        debug::sample = sample;
        debug::nest = 0;
#endif // ENABLE_DEBUG
    }

    inline
    void boiler ()
    {
#if ENABLE_DEBUG
        std::cout << debug::x << "," << debug::y << ":";
        std::cout << debug::sample << ":" << debug::nest << "::";
#endif // ENABLE_DEBUG
    }

    inline
    void add (const char* id, const vec3& v)
    {
#if ENABLE_DEBUG
        if(on){
            boiler();
            std::cout << id << " " << v << "\n";
        }
#endif // ENABLE_DEBUG
    }

    inline
    void add (const char* id, const float v)
    {
#if ENABLE_DEBUG
        if(on){
            boiler();
            std::cout << id << " " << v << "\n";
        }
#endif // ENABLE_DEBUG
    }

    inline
    void up ()
    {
#if ENABLE_DEBUG
        nest++;
#endif // ENABLE_DEBUG
    }

    inline
    void down ()
    {
#if ENABLE_DEBUG
        nest--;
#endif // ENABLE_DEBUG
    }
}


struct Ray
{
    vec3 o;
    vec3 d;
    float tmin, tmax;

    Ray (const vec3& o, const vec3& d,
         float tmin = 0, float tmax = INFINITY)
        : o(o), d(d), tmin(tmin), tmax(tmax)
    { }

    Ray transform (const Transform& xform) const
    {
        return Ray(xform.point(o), xform.vector(d), tmin, tmax);
    }
};

class BBox
{
public:
    vec3 min, max;

    BBox ();
    BBox (const vec3& min, const vec3& max);
    void extend (const vec3& v);
    bool intersect (const Ray& ray) const;

    /// Bounding box dimensions.
    vec3 dim () const { return max - min; }
};

class Material;
class Primitive;

struct Isect
{
    vec3 p;
    vec3 n;
    Material* mat;
    Spectrum Le; // this is oversimplified
    const Primitive* prim;
};


class Shape
{
public:
	virtual ~Shape () {}
    /// The parameters self and inside_self are used to prevent surface acne.
    ///
    /// @par self   Set to true iff the primitive of the previous intersection is
    ///             the same as is being tested now.
    /// @par inside_self    Set to true iff self is true and r was shot from
    ///                     the previous intersection in a direction opposite
    ///                     the surface normal.
    virtual bool intersect (Ray& r, Isect* isect, bool self, bool inside_self) = 0;

    virtual BBox get_bbox () const = 0;
};


class BSDF
{
public:
    virtual ~BSDF () {}

    /// @param wo [in]  exiting vector in tangent space, normalized
    /// @param wi [out] entering vector in tangent space, normalized
    /// @return reflectance f(wo,wi)
    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv, float* pdf) const = 0;
};


class Material
{
public:
    virtual ~Material () {}
    // virtual std::unique_ptr<BSDF> get_bsdf (const vec3& p) const = 0;
    virtual std::unique_ptr<BSDF> get_bsdf (const vec3& p, const vec2& u) const = 0;
};

class Primitive
{
public:
    virtual ~Primitive () {}
    /// @par prev The previous intersection (nullptr if r is a camera ray).
    ///           Used in surface acne prevention.
    virtual bool intersect (Ray& r, Isect* isect, const Isect* prev) const = 0;
};



class ListAggregate : public Primitive
{
public:
    std::vector<std::shared_ptr<const Primitive>> prims;

    void add (std::shared_ptr<const Primitive> p)
    {
        prims.push_back(p);
    }

    bool intersect (Ray& r, Isect* isect, const Isect* prev) const
    {
        bool hit = false;
        for (auto& prim : prims) {
            hit |= prim->intersect(r, isect, prev);
        }
        return hit;
    }
};

class GeometricPrimitive : public Primitive
{
public:
    shared_ptr<Material> mat;
    shared_ptr<Shape> shape;
    Transform world_from_prim;
    mutable Transform prim_from_world;
    Spectrum Le;

    bool intersect (Ray& r, Isect* isect, const Isect* prev) const
    {
        prim_from_world = inverse(world_from_prim); // FIXME: dont do this every time!

        Ray ro = r.transform(prim_from_world);
        Isect is2;

        if (prev && prev->prim == this) {
            bool inside = ( dot(r.d, prev->n) < 0 );
            if (!shape->intersect(ro, &is2, true, inside)) {
                return false;
            }
        }
        else {
            if (!shape->intersect(ro, &is2, false, false)) {
                return false;
            }
        }

        r.tmax = ro.tmax;
        isect->p = world_from_prim.point(is2.p);
        isect->n = normalize(world_from_prim.normal(is2.n));
        isect->mat = mat.get();
        isect->Le = Le;
        isect->prim = this;
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


class Skylight
{
public:
    virtual Spectrum sample (const vec3& direction) const = 0;
    Spectrum sample (const Ray& ray) const
    {
        return sample(ray.d);
    }
};

class Scene
{
public:
    shared_ptr<Primitive> primitives;
    shared_ptr<Camera> camera;
    shared_ptr<Skylight> skylight;

    bool intersect (Ray& ray, Isect* isect, const Isect* prev) const
    {
        return primitives->intersect(ray, isect, prev);
    }
};

class SurfaceIntegrator
{
public:
    /// The outgoing radiance along the ray,
    /// or the incoming radiance at the ray origin.
    virtual Spectrum Li (Ray& ray, const Scene* scene, Sample& sample, const Isect* prev=nullptr) = 0;

    static SurfaceIntegrator* make ();
};


#endif /* GRAY_HPP */
