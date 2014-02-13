
#include "film.hpp"
#include <iostream>
#include "materials.hpp"
#include "util.hpp"
#include <cstring>
#include <thread>
#include <list>

class Texture
{
public:
    Spectrum sample (const vec3& pos) const;

    Spectrum R;
};


// #include "lisc_gray.hpp"
#include "lisc.hpp"

Scene* load (const char* filename)
{
    Scene* scn = new Scene();
    // Evaluator ev;
    // LiscLinAlg* la = new LiscLinAlg(&ev);
    // LiscGray* lg = LiscGray::create(&ev, la);
    // ev.evaluate_file(filename);

    // auto* list = new ListAggregate();
    // for (auto p : lg->primitives) {
    //     list->add(p);
    // }
    // scn->primitives = list;

    // delete lg;
    // delete la;
    
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
        if (f == Spectrum(0.0f)) {
            terminated++;
            return Spectrum(0.0f);
        }
        vec3 wi = inverse(tangent_from_world).vector(wi_t);

        constexpr float russian_p = 0.99;
        if (frand() > russian_p) {
            terminated++;
            return Spectrum(0.0f);
        }

        Ray newray = Ray(isect.p, wi);
        Isect newisect;
        Spectrum Li;
        if (isect.Le != Spectrum(0.0f)) {
            arrived++;
            Li = Spectrum(0.0f);
        }
        else if (scene->intersect(newray, &newisect))  {
            Li = this->Li(newray, newisect, scene);
        }
        else {
            Li = Spectrum(0.0f);//fmax(dot(wo, normalize(vec3(-10,7,3))), 0.f) * 10.0f);
        }
        Li /= russian_p;

        // Light transport equation.
        Spectrum L = isect.Le + f * Li * abs_cos_theta(wi_t);
        return L;
    }
};


void render_block (Scene* scene, int spp,
                   int fullresx, int fullresy,
                   int xofs, int yofs,
                   Film* filmp)
{
    Film& film = *filmp;
    PathIntegrator* surf_integ = new PathIntegrator();

    for (int yp = 0; yp < film.yres; yp++) {
        for (int xp = 0; xp < film.xres; xp++) {
            for (int s = 0; s < spp; s++) {
                float xf = (xp+frand()) / (float)film.xres;
                float yf = (yp+frand()) / (float)film.yres;

                vec3 orig(0,0,2);
                // vec3 dir(normalize(vec3(xf*2-1, yf*2-1, -1)));
                vec3 dir(normalize(vec3(((xofs+xf*film.xres)/fullresx)*2-1,
                                        ((yofs+yf*film.yres)/fullresy)*2-1, -1)));

                Ray ray(orig, dir);
                Isect isect;
                Spectrum L(0.0f);

                if (scene->intersect(ray, &isect)) {
                    L = surf_integ->Li(ray, isect, scene);
                }
                else L = Spectrum(0,0,0);

                film.add_sample(xf,yf, L);
            }
        }
    }
}



class RenderTask
{
public:
    int bx, by;
    int block_size;
    int resx, resy;
    int spp;
    Scene* scene;
    int blw, blh;
    std::unique_ptr<Film> film;
    std::thread th;
    RenderTask () {}
    RenderTask (int bx, int by, int block_size,
                int resx, int resy, int spp,
                Scene* scene)
        : bx(bx), by(by), block_size(block_size),
        resx(resx), resy(resy), spp(spp), scene(scene),
        blw(std::min(block_size, resx - bx*block_size)),
        blh(std::min(block_size, resy - by*block_size))
    { }

    void start ()
    {
        printf("Block %d,%d (%dx%d)\n", bx, by, blw, blh);
        film.reset(new Film(blw, blh));
        th = std::thread(render_block, scene, spp,
                         resx, resy,
                         bx * block_size, by * block_size,
                         film.get());
    }

};





Value evaluate_shape (Value& val, List& args);


Value evaluate_xform (Value& val, List& args)
{
    Transform T;
    std::cout << "evalxform " << args << std::endl;

    for (auto factor : args) {
        std::cout << "evalxform factor " << factor << std::endl;
        if (!factor.is_list()) throw std::runtime_error("evaluate_xform: item not a list");

        auto aa = factor.list;
        auto name = *pop<std::string>(aa);
        if (name == "translate") {
            auto v = *pop<glm::vec3>(aa);
            T = T * Transform::translate(v);
        }
        else if (name == "scale") {
            auto v = *pop<double>(aa);
            T = T * Transform::scale(glm::vec3(v));
        }
        else if (name == "rotate") {
            auto angle = *pop<double>(aa);
            auto axis = *pop<glm::vec3>(aa);
            T = T * Transform::rotate(angle, axis);
        }
        else {
            throw std::runtime_error("invalid transform name");
        }
    }

    return Value({"_xform", new Transform(T)});
}


Value evaluate_prim (Value& val, List& args)
{
    auto* p = new GeometricPrimitive();
    p->mat = pop_attr<Material>("_material", args).get();
    p->shape = pop_attr<Shape>("_shape", args).get();
    p->world_from_prim = *pop_attr<Transform>("_xform", args);
    p->Le = *pop_attr<Spectrum>("emit", std::shared_ptr<Spectrum>(new Spectrum(0)), args);

    return Value({"_prim", dynamic_cast<Primitive*>(p)});
}





Scene* lmain ()
{
    Value* w = new Value({
        {"x", {"+", 1.2, 0.8, 1.0}},
        {"material", "diffuse", {"R", new Spectrum(1.2, 0.8, 1.0)}},
        {"prim",
            {"shape", "sphere"},
            {"material", "diffuse", {"R", new Spectrum(1.0f)}},
            {"xform", {"translate", new glm::vec3(0,0,-1)}},
            {"emit", new Spectrum(1.0f)},
        }
    });

    auto e = Evaluator();
    std::cout << *w << std::endl;
    e.evaluate(*w);
    std::cout << *w << std::endl;

    std::shared_ptr<Material> m = nullptr;
    while ( (m = pop_attr<Material>("_material", nullptr, w->list)) ) {
        std::cout << "got " << m << std::endl;
    }

    Scene* scene = new Scene();
    ListAggregate* agg = new ListAggregate();
    std::shared_ptr<Primitive> p = nullptr;
    while ( (p = pop_attr<Primitive>("_prim", nullptr, w->list)) ) {
        std::cout << "got prim " << p << std::endl;
        auto* pp = new std::shared_ptr<Primitive>(p);
        std::cout << "    pp " << pp << std::endl;
        agg->add(p.get());
    }
    scene->primitives = agg;
    return scene;
}

int main (int argc, char* argv[])
{
    int resx = 256;
    int resy = 256;
    int spp = 100;
    unsigned int thread_count = 3;
    // const char* input_filename = "test1.scene";
    const char* output_filename = "out";

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-r") == 0) {
            resx = atol(argv[++i]);
            resy = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "-s") == 0) {
            spp = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "-o") == 0) {
            output_filename = argv[++i];
        }
        else if (strcmp(argv[i], "-m") == 0) {
            thread_count = atol(argv[++i]);
        }
        else {
            // input_filename = argv[i];
        }
    }



    Scene* scene = nullptr;
    try {
        scene = lmain();//load(input_filename);

        printf("Resolution: %d x %d\n", resx, resy);
        printf("Samples per pixel: %d\n", spp);

        PathIntegrator* surf_integ = new PathIntegrator();

        int block_size = 32;

        Film wholefilm(resx, resy);
        std::vector<RenderTask*> tasks;
        for (int by = 0; by < (resy + block_size-1) / block_size; by++) {
            for (int bx = 0; bx < (resx + block_size-1) / block_size; bx++) {
                tasks.push_back(new RenderTask(bx, by, block_size,
                                resx, resy, spp,
                                scene));
            }
        }

        std::list<RenderTask*> active;
        while (active.size() < thread_count && tasks.size() > 0) {
            auto* t = tasks.back();
            tasks.pop_back();
            t->start();
            active.push_back(t);
        }
        while (active.size() > 0) {
            auto* t = active.front();
            active.pop_front();
            t->th.join();
            wholefilm.merge(*t->film, t->bx*block_size, t->by*block_size);
            delete t;
            if (tasks.size() > 0) {
                auto* t = tasks.back();
                tasks.pop_back();
                t->start();
                active.push_back(t);
            }
        }


        int paths = wholefilm.xres*wholefilm.yres*spp;
        printf("Rays shot: %d\n", surf_integ->rays);
        printf("Rays terminated: %d (%.0f%%)\n", surf_integ->terminated, surf_integ->terminated / (float)surf_integ->rays * 100);
        printf("Paths shot: %d\n", paths);
        printf("Paths reached light: %d (%.0f%%)\n", surf_integ->arrived, surf_integ->arrived / (float)paths * 100);
        printf("Paths terminated: %d (%.0f%%)\n", surf_integ->terminated, surf_integ->terminated / (float)paths * 100);
        printf("Avg rays/path: %.1f\n", (float)surf_integ->rays / paths);

        char filename[256];
        sprintf(filename, "%s.png", output_filename);
        wholefilm.save(filename);
        sprintf(filename, "%s.float", output_filename);
        wholefilm.save_float(filename);
        sprintf(filename, "%s.hdr", output_filename);
        wholefilm.save_rgbe(filename);

    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::string& e) {
        std::cerr << "Exception caught: " << e << std::endl;
        return 1;
    }
    catch (const char* e) {
        std::cerr << "Exception caught: " << e << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown exception caught!" << std::endl;
        return 1;
    }


    return 0;
}
