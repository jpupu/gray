
#include "film.hpp"
#include <iostream>
#include "util.hpp"
#include <cstring>
#include <thread>
#include <list>
#include "timer.hpp"
#include "malloc.hpp"
#include "renderjob.hpp"

class Texture
{
public:
    Spectrum sample (const vec3& pos) const;

    Spectrum R;
};


namespace debug {
    int x,y,sample,nest,on=0;
}


// class SurfaceIntegrator
// {
// public:
//     /// The outgoing radiance along the ray,
//     /// or the incoming radiance at the ray origin.
//     virtual Spectrum Li (Ray& ray, const Scene* scene) = 0;
// };

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

    virtual Spectrum Li (Ray& ray, const Scene* scene, Sample& sample, const Isect* prev)
    {
        debug::up();

        debug::add("------------------------------------------", 0);
        debug::add("Li: ray.o", ray.o);
        debug::add("Li: ray.d", ray.d);

        constexpr float russian_p = 0.99;
        if (sample.randf() > russian_p) {
            terminated++;
            debug::down();
            return Spectrum(0.0f);
        }

        rays++;

        Spectrum L;
        Spectrum Le(0.0f);
        Isect isect;
        if (scene->intersect(ray, &isect, prev)) {
            debug::add("Li: ray.t", ray.tmax);
            debug::add("Li: isect.p", isect.p);
            debug::add("Li: isect.n", isect.n);

            // The ray hit a point in the scene.
            // ----------------------------------
            std::unique_ptr<BSDF> bsdf = isect.mat->get_bsdf(isect.p, sample.rand2f());
            Transform tangent_from_world = build_tangent_from_world(isect.n);
            vec3 wo_t = tangent_from_world.vector(-ray.d);
            vec3 wi_t;
            float pdf;
            debug::add("Li: wo_t", wo_t);
            debug::add("Li: wi_t", wi_t);

            Spectrum f = bsdf->sample(wo_t, &wi_t, sample.get2d(), &pdf);
            vec3 wi = inverse(tangent_from_world).vector(wi_t);

            debug::add("Li: wo", -ray.d);
            debug::add("Li: wi", wi);
            Ray newray = Ray(isect.p, wi);
            Spectrum Li = this->Li(newray, scene, sample, &isect);

            // Light transport equation.
            L = isect.Le + f * Li * abs_cos_theta(wi_t) / pdf;
            debug::add("Le", Le);
            debug::add("f", f);
            debug::add("Li", Li);
            debug::add("cos", abs_cos_theta(wi_t));
            debug::add("pdf", pdf);
            debug::add("f cos pdf", f * abs_cos_theta(wi_t) / pdf);
            debug::add("isect.p", isect.p);
            debug::add("ray.d", ray.d);
        }
        else {
            // The ray did not hit the scene.
            // -------------------------------
            L = scene->skylight->sample(ray);
        }

        L = L / russian_p;
        debug::add("-- L", L);
        debug::down();
        return L;
    }
};

SurfaceIntegrator* SurfaceIntegrator::make ()
{
    return new PathIntegrator();
}



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
    int block_size = 32;
    int single_block_x = -1;
    int single_block_y = -1;
    unsigned int thread_count = 3;
    const char* input_filename = "test1.lisc";
    const char* output_filename = "out";
    std::string sampler_name = "random";

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
        else if (strcmp(argv[i], "-b") == 0) {
            block_size = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "-d") == 0) {
            debug::on = 1;
        }
        else if (strcmp(argv[i], "-S") == 0) {
            single_block_x = atol(argv[++i]);
            single_block_y = atol(argv[++i]);
            block_size = 1;
        }
        else if (strcmp(argv[i], "--sampler") == 0) {
            sampler_name = std::string(argv[++i]);
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

        printf("Resolution: %d x %d\n", resx, resy);
        printf("Samples per pixel: %d\n", spp);

        Film wholefilm(resx, resy);
        threaded_render::Job job(thread_count, *scene, wholefilm);
        std::vector<threaded_render::TaskDesc> tasks;
        if (single_block_x != -1) {
            tasks.push_back(threaded_render::TaskDesc{
                            single_block_x, single_block_y,
                            block_size, block_size,
                            spp, sampler_name});
        }
        else {
            for (int by = 0; by < (resy + block_size-1) / block_size; by++) {
                for (int bx = 0; bx < (resx + block_size-1) / block_size; bx++) {
                    int xofs = bx * block_size;
                    int yofs = by * block_size;
                    int xsize = (resx - xofs < block_size) ? resx - xofs : block_size;
                    int ysize = (resy - yofs < block_size) ? resy - yofs : block_size;
                    tasks.push_back(threaded_render::TaskDesc{
                                    xofs, yofs,
                                    xsize, ysize,
                                    spp, sampler_name});
                }
            }
        }
        std::random_shuffle(tasks.begin(), tasks.end());

#ifdef DEBUG_MALLOC
        std::cout << "pre-render mem usage "<<get_mem_usage()<<" bytes in allocations "<<get_mem_allocs()<<"\n";
#endif

        Timer render_timer;
        Timer preview_timer;
        render_timer.start();

        int total_tasks = tasks.size();
        int completed_tasks = 0;
        job.set_callback([&](const threaded_render::Task& task) {
            ++completed_tasks;
            if (preview_timer.snap() > 2.0) {
                std::cout << "completed: " << completed_tasks << " / " << total_tasks << "\r";
                char filename[256];
                sprintf(filename, "%s.hdr", output_filename);
                wholefilm.save_rgbe(filename);
                preview_timer.start();
            }
        });
        for (auto& t : tasks) {
            job.add_task(t);
        }
        job.finish();
        render_timer.stop();

        // int paths = wholefilm.xres*wholefilm.yres*spp;
        // // printf("Rays shot: %d\n", surf_integ->rays);
        // // printf("Rays terminated: %d (%.0f%%)\n", surf_integ->terminated, surf_integ->terminated / (float)surf_integ->rays * 100);
        // printf("Paths shot: %d\n", paths);
        // printf("NaNs: %d\n", nans);
        // printf("Negatives: %d\n", negs);
        // // printf("Paths reached light: %d (%.0f%%)\n", surf_integ->arrived, surf_integ->arrived / (float)paths * 100);
        // // printf("Paths terminated: %d (%.0f%%)\n", surf_integ->terminated, surf_integ->terminated / (float)paths * 100);
        // // printf("Avg rays/path: %.1f\n", (float)surf_integ->rays / paths);

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
