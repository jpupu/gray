
#include "film.hpp"
#include <iostream>
#include "util.hpp"
#include <cstring>
#include <thread>
#include <list>
#include "timer.hpp"
#include "malloc.hpp"

class Texture
{
public:
    Spectrum sample (const vec3& pos) const;

    Spectrum R;
};





class SurfaceIntegrator
{
public:
    /// The outgoing radiance along the ray,
    /// or the incoming radiance at the ray origin.
    virtual Spectrum Li (Ray& ray, const Scene* scene) = 0;
};

// class SimpleIntegrator : public SurfaceIntegrator
// {
//     virtual Spectrum Li (const Ray& ray, const Isect& isect, const Scene* scene)
//     {
//         std::unique_ptr<BSDF> bsdf = isect.mat->get_bsdf(isect.p);
//         Transform tangent_from_world = build_tangent_from_world(isect.n);
//         vec3 wo_t = tangent_from_world.vector(-ray.d);
//         vec3 wi_t;
//         float pdf;
//         Spectrum f = bsdf->sample(wo_t, &wi_t, glm::vec2(frand(),frand()), &pdf);
//         vec3 wo = inverse(tangent_from_world).vector(wi_t);
//         Spectrum L = f * (float)fmax(dot(wo, normalize(vec3(-10,7,3))), 0.f) * 10.0f / pdf;
//         return L;
//     }
// };

// class NormalIntegrator : public SurfaceIntegrator
// {
//     virtual Spectrum Li (const Ray& ray, const Isect& isect, const Scene* scene)
//     {
//         return Spectrum(isect.n) * .5f + Spectrum(.5f);
//     }
// };

class PathIntegrator : public SurfaceIntegrator
{
public:
    int rays;
    int terminated;
    int arrived;

    PathIntegrator ()
        : rays(0), terminated(0), arrived(0)
    { }

    virtual Spectrum Li (Ray& ray, const Scene* scene)
    {
        constexpr float russian_p = 0.99;
        if (frand() > russian_p) {
            terminated++;
            return Spectrum(0.0f);
        }

        rays++;

        Spectrum L;
        Spectrum Le(0.0f);
        Isect isect;
        if (scene->intersect(ray, &isect)) {
            // The ray hit a point in the scene.
            // ----------------------------------
            std::unique_ptr<BSDF> bsdf = isect.mat->get_bsdf(isect.p);
            Transform tangent_from_world = build_tangent_from_world(isect.n);
            vec3 wo_t = tangent_from_world.vector(-ray.d);
            vec3 wi_t;
            float pdf;

            Spectrum f = bsdf->sample(wo_t, &wi_t, glm::vec2(frand(),frand()), &pdf);
            vec3 wi = inverse(tangent_from_world).vector(wi_t);

            Ray newray = Ray(isect.p, wi);
            Spectrum Li = this->Li(newray, scene);

            // Light transport equation.
            L = isect.Le + f * Li * abs_cos_theta(wi_t) / pdf;
        }
        else {
            // The ray did not hit the scene.
            // -------------------------------
            L = scene->skylight->sample(ray);
        }

        L = L / russian_p;

        return L;
    }
};




float xxx;
void render_block (Scene* scene, int spp,
                   int fullresx, int fullresy,
                   int xofs, int yofs,
                   Film* filmp,
                   int* nans, int* negs)
{
    srand(xofs+yofs*fullresx);
    Film& film = *filmp;
    std::unique_ptr<PathIntegrator> surf_integ(new PathIntegrator());

    Camera* cam = scene->camera.get();

    for (int yp = 0; yp < film.yres; yp++) {
        for (int xp = 0; xp < film.xres; xp++) {
            for (int s = 0; s < spp; s++) {
                float rx = xp + frand();
                float ry = yp + frand();

                float xf = rx / (float)film.xres;
                float yf = ry / (float)film.yres;

                float xfn = (xofs + rx) / fullresx;
                float yfn = (yofs + ry) / fullresy;
                
                Ray ray( cam->generate_ray(xfn,yfn, frand(),frand()) );

                Spectrum L = surf_integ->Li(ray, scene);

                // NaN counter.
                for (int i = 0; i < 3; i++) {
                    nans += std::isnan(L.x) || std::isnan(L.y) || std::isnan(L.z);
                    negs += (L.x < 0.0f) || (L.y < 0.0f) || (L.z < 0.0f);
                }

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
    int nans, negs;
    std::unique_ptr<Film> film;
    std::thread th;
    RenderTask () {}
    RenderTask (int bx, int by, int block_size,
                int resx, int resy, int spp,
                Scene* scene)
        : bx(bx), by(by), block_size(block_size),
        resx(resx), resy(resy), spp(spp), scene(scene),
        blw(std::min(block_size, resx - bx*block_size)),
        blh(std::min(block_size, resy - by*block_size)),
        nans(0), negs(0)
    { }

    void start ()
    {
        // printf("Block %d,%d (%dx%d)\n", bx, by, blw, blh);
        film.reset(new Film(blw, blh));
        th = std::thread(render_block, scene, spp,
                         resx, resy,
                         bx * block_size, by * block_size,
                         film.get(), &nans, &negs);
    }

    void sync_run ()
    {
        film.reset(new Film(blw, blh));
        render_block(scene, spp,
                     resx, resy,
                     bx * block_size, by * block_size,
                     film.get(), &nans, &negs);
    }
};


#include "lisc_gray.hpp"
#include "lisc_linalg.hpp"

Scene* evaluate_scene (Value& description) {
    auto e = Evaluator();
    e.add_set(evaluate_linalg);
    e.add_set(evaluate_gray);
    // std::cout << description << std::endl;
    e.evaluate(description);
    // std::cout << description << std::endl;

    Scene* scene = new Scene();
    std::shared_ptr<ListAggregate> agg = std::make_shared<ListAggregate>();
    std::shared_ptr<Primitive> p = nullptr;
    while ( (p = pop_attr<Primitive>("_prim", nullptr, description.list)) ) {
        agg->add(p);
    }
    scene->primitives = agg;
    scene->camera = pop_attr<Camera>("_camera", description.list);
    scene->skylight = pop_attr<Skylight>("_skylight", description.list);
    return scene;
}

Scene* load (const char* filename)
{
    Value w( parse_file(filename) );
    return evaluate_scene(w);
}



int main (int argc, char* argv[])
{
    int resx = 256;
    int resy = 256;
    int spp = 100;
    unsigned int thread_count = 3;
    const char* input_filename = "test1.lisc";
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
        else if (strcmp(argv[i], "-M") == 0) {
            set_mem_limit(atol(argv[++i]) * 1000000);
        }
        else {
            input_filename = argv[i];
        }
    }

#ifdef DEBUG_MALLOC
    std::cout << "baseline mem usage "<<get_mem_usage()<<" bytes in allocations "<<get_mem_allocs()<<"\n";
#endif

    try {
        std::unique_ptr<Scene> scene = nullptr;
        Timer load_timer;

        load_timer.start();
        scene.reset(load(input_filename));
        load_timer.stop();
        // Scene* scene = nullptr;
        // scene = load(input_filename);

        printf("Resolution: %d x %d\n", resx, resy);
        printf("Samples per pixel: %d\n", spp);

        // unique_ptr<PathIntegrator> surf_integ( new PathIntegrator() );

        int block_size = 32;

        Film wholefilm(resx, resy);
        std::vector<RenderTask*> tasks;
        for (int by = 0; by < (resy + block_size-1) / block_size; by++) {
            for (int bx = 0; bx < (resx + block_size-1) / block_size; bx++) {
                tasks.push_back(new RenderTask(bx, by, block_size,
                                resx, resy, spp,
                                scene.get()));
                                // scene));
            }
        }
        std::random_shuffle(tasks.begin(), tasks.end());

#ifdef DEBUG_MALLOC
        std::cout << "pre-render mem usage "<<get_mem_usage()<<" bytes in allocations "<<get_mem_allocs()<<"\n";
#endif

        Timer render_timer;
        Timer preview_timer;
        render_timer.start();

        std::list<RenderTask*> active;
        int total_tasks = tasks.size();
        int completed_tasks = 0;
        int nans = 0, negs = 0;
#ifdef PROFILING
        for (auto t : tasks) {
            t->sync_run();
            wholefilm.merge(*t->film, t->bx*block_size, t->by*block_size);
            nans += t->nans;
            negs += t->negs;
        }
#else
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
            nans += t->nans;
            negs += t->negs;
            delete t;
            if (tasks.size() > 0) {
                auto* t = tasks.back();
                tasks.pop_back();
                t->start();
                active.push_back(t);
            }

            ++completed_tasks;
            std::cout << "completed: " << completed_tasks << " / " << total_tasks << "\r";

            if (preview_timer.snap() > 2.0) {
                char filename[256];
                sprintf(filename, "%s.hdr", output_filename);
                wholefilm.save_rgbe(filename);
                preview_timer.start();
            }


#ifdef DEBUG_MALLOC
            std::cout << "mem usage: "<<get_mem_usage()<<" bytes (peak "<<get_peak_mem_usage()<<" bytes) in allocations "<<get_mem_allocs()<<"\n";
#endif
        }
        std::cout << std::endl;
#endif // PROFILING
        
        render_timer.stop();

        int paths = wholefilm.xres*wholefilm.yres*spp;
        // printf("Rays shot: %d\n", surf_integ->rays);
        // printf("Rays terminated: %d (%.0f%%)\n", surf_integ->terminated, surf_integ->terminated / (float)surf_integ->rays * 100);
        printf("Paths shot: %d\n", paths);
        printf("NaNs: %d\n", nans);
        printf("Negatives: %d\n", negs);
        // printf("Paths reached light: %d (%.0f%%)\n", surf_integ->arrived, surf_integ->arrived / (float)paths * 100);
        // printf("Paths terminated: %d (%.0f%%)\n", surf_integ->terminated, surf_integ->terminated / (float)paths * 100);
        // printf("Avg rays/path: %.1f\n", (float)surf_integ->rays / paths);

        std::cout << std::endl;
        std::cout << "Loading time   " << load_timer << std::endl;
        std::cout << "Rendering time " << render_timer << std::endl;

        char filename[256];
        // sprintf(filename, "%s.png", output_filename);
        // wholefilm.save(filename);
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

#ifdef DEBUG_MALLOC
    std::cout << std::endl;
    std::cout << "PEAK mem "<<get_peak_mem_usage()<<" bytes, peak allocations "<<get_peak_mem_allocs()<<"\n";
    std::cout << std::endl;
    std::cout << "TOTAL mem usage "<<get_total_mem_usage()<<" bytes in allocations "<<get_total_mem_allocs()<<"\n";
    std::cout << "CURRENT mem usage "<<get_mem_usage()<<" bytes in allocations "<<get_mem_allocs()<<"\n";
    std::cout << "unfreed allocations "<<(100*get_mem_allocs()/get_total_mem_allocs())<<"%\n";
#endif

    return 0;
}
