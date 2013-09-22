
#include "film.hpp"
#include <iostream>
#include "materials.hpp"
#include "util.hpp"




class Texture
{
public:
    Spectrum sample (const vec3& pos) const;

    Spectrum R;
};


#include "lisc_gray.hpp"

Scene* load (const char* filename)
{
    Scene* scn = new Scene();
    Evaluator ev;
    LiscLinAlg* la = new LiscLinAlg(&ev);
    LiscGray* lg = LiscGray::create(&ev, la);
    ev.evaluate_file(filename);

    auto* list = new ListAggregate();
    for (auto p : lg->primitives) {
        list->add(p);
    }
    scn->primitives = list;

    delete lg;
    delete la;
    
    return scn;
}

class SurfaceIntegrator
{
public:
    /// The outgoing radiance at the intersection point,
    /// or the incoming radiance at the ray origin.
    virtual Spectrum Li (const Ray& ray, const Isect& isect, const Scene* scene) = 0;
};

class SimpleIntegrator : public SurfaceIntegrator
{
    virtual Spectrum Li (const Ray& ray, const Isect& isect, const Scene* scene)
    {
        BSDF* bsdf = isect.mat->get_bsdf(isect.p);
        Transform tangent_from_world = build_tangent_from_world(isect.n);
        vec3 wo_t = tangent_from_world.vector(-ray.d);
        vec3 wi_t;
        Spectrum f = bsdf->sample(wo_t, &wi_t, glm::vec2(frand(),frand()));
        delete bsdf;
        vec3 wo = inverse(tangent_from_world).vector(wi_t);
        Spectrum L = f * (float)fmax(dot(wo, normalize(vec3(-10,7,3))), 0.f) * 10.0f;
        return L;
    }
};

class NormalIntegrator : public SurfaceIntegrator
{
    virtual Spectrum Li (const Ray& ray, const Isect& isect, const Scene* scene)
    {
        return Spectrum(isect.n) * .5f + Spectrum(.5f);
    }
};

class PathIntegrator : public SurfaceIntegrator
{
public:
    int rays;
    int terminated;
    int arrived;

    PathIntegrator ()
        : rays(0), terminated(0), arrived(0)
    { }

    virtual Spectrum Li (const Ray& ray, const Isect& isect, const Scene* scene)
    {
        rays++;

        BSDF* bsdf = isect.mat->get_bsdf(isect.p);
        Transform tangent_from_world = build_tangent_from_world(isect.n);
        vec3 wo_t = tangent_from_world.vector(-ray.d);
        vec3 wi_t;
        Spectrum f = bsdf->sample(wo_t, &wi_t, glm::vec2(frand(),frand()));
        delete bsdf;
        vec3 wo = inverse(tangent_from_world).vector(wi_t);

        constexpr float russian_p = 0.99;
        if (frand() > russian_p) {
            terminated++;
            return Spectrum(0.0f);
        }

        Ray newray = Ray(isect.p, wo);
        Isect newisect;
        Spectrum Linc;
        if (isect.Le != Spectrum(0.0f)) {
            arrived++;
            Linc = Spectrum(0.0f);
        }
        else if (scene->intersect(newray, &newisect))  {
            Linc = Li(newray, newisect, scene);
        }
        else {
            Linc = Spectrum(0.0f);//fmax(dot(wo, normalize(vec3(-10,7,3))), 0.f) * 10.0f);
        }
        Spectrum L = f * Linc / russian_p + isect.Le;
        return L;
    }
};



int main (int argc, char* argv[])
{
    // Film film(512,512);
    Film film(256,256);
#if 1
    film.load_float("out.float");
#else
    Scene* scene = nullptr;
    try {
        scene = load("test1.scene");
    }
    catch (std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }

    int spp = 200 * 20;//150 * 10;

    PathIntegrator* surf_integ = new PathIntegrator();

    for (int yp = 0; yp < film.yres; yp++) {
        for (int xp = 0; xp < film.xres; xp++) {
            for (int s = 0; s < spp; s++) {
                float xf = (xp+frand()) / (float)film.xres;
                float yf = (yp+frand()) / (float)film.yres;

                vec3 orig(0,0,2);
                vec3 dir(normalize(vec3(xf*2-1, yf*2-1, -1)));

                Ray ray(orig, dir);
                Isect isect;
                Spectrum L(0.0f);

                if (scene->intersect(ray, &isect)) {
                    L = surf_integ->Li(ray, isect, scene);
                }
                else L = Spectrum(0,1,0);

                film.add_sample(xf,yf, L);
            }
        }
    }
    int paths = film.xres*film.yres*spp;
    printf("Rays shot: %d\n", surf_integ->rays);
    printf("Rays terminated: %d (%.0f%%)\n", surf_integ->terminated, surf_integ->terminated / (float)surf_integ->rays * 100);
    printf("Paths shot: %d\n", paths);
    printf("Paths reached light: %d (%.0f%%)\n", surf_integ->arrived, surf_integ->arrived / (float)paths * 100);
    printf("Paths terminated: %d (%.0f%%)\n", surf_integ->terminated, surf_integ->terminated / (float)paths * 100);
    printf("Avg rays/path: %.1f\n", (float)surf_integ->rays / paths);

    film.save_float("out.float");
#endif
    film.save("out.png");
    film.save_rgbe("out.hdr");
    // film.save_float("out.float");


    return 0;
}
