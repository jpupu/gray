
#include "film.hpp"
#include <iostream>
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
        film_w = w_mm / 1000;
        film_h = h_mm / 1000;
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


class PinholeCamera : public Camera
{
public:
    PinholeCamera ()
    {
        set_fov(90);
    }

    float film_d;

    void set_fov (float horizontal_fov_degrees)
    {
        film_d = (film_w/2) / tan(horizontal_fov_degrees/2 * M_PI/180);
    }


    virtual std::pair<vec3,vec3> get_vector (float x, float y, float u, float v) const
    {
        vec3 I(x, y, film_d);
        return std::make_pair(vec3(0,0,0), normalize(-I));
    }
    
};

std::ostream& operator<< (std::ostream& os, const glm::vec3& v)
{
    os << "<" << v.x << ", " << v.y << ", " << v.z << ">";
    return os;
}

class ThinLensCamera : public Camera
{
public:
    ThinLensCamera ()
    {
        set_focal_length(55);
        set_focus_distance(10);
        set_f_number(5.6);
    }

    void set_focal_length (float mm)
    {
        f = mm / 1000;
    }

    void set_focus_distance (float m)
    {
        d_o = -m;
    }

    void set_f_number (float N)
    {
        this->N = N;
    }

    // focal length
    // (represents optical power i.e. magnification)
    float f;
    // object distance (must be negative)
    // (represents focus distance)
    float d_o;
    // the f-number;
    float N;

    virtual std::pair<vec3,vec3> get_vector (float x, float y, float u, float v) const
    {
        // magnification
        float M = f / (f - d_o);
        // image distance
        float d_i = -M * d_o;
        // lens diameter
        float D = f / N;

        vec3 I(x, y, d_i);
        vec3 P(u*D/2, v*D/2, 0.0f);
        vec3 O(I.x/M, I.y/M, d_o);

        // std::cout << "M = " << M << ",  di = " << d_i << std::endl;
        // std::cout << O << std::endl;

        return std::make_pair(P, normalize(O - P));
    }

};
float xxx;
void render_block (Scene* scene, int spp,
                   int fullresx, int fullresy,
                   int xofs, int yofs,
                   Film* filmp)
{
    Film& film = *filmp;
    PathIntegrator* surf_integ = new PathIntegrator();

    // PinholeCamera* cam = new PinholeCamera();
    // cam->film_w = 0.04;
    // cam->film_h = 0.04;
    // cam->film_d = 0.02;
    ThinLensCamera* cam = new ThinLensCamera();
    cam->set_xform( Transform::translate(vec3(0,0,6)) *  Transform::rotate(10, vec3(0,0,1)) );
    cam->set_film(36, 36);
    cam->set_focal_length(55);
    cam->set_focus_distance(xxx);
    cam->set_f_number(2.8);

    for (int yp = 0; yp < film.yres; yp++) {
        for (int xp = 0; xp < film.xres; xp++) {
            for (int s = 0; s < spp; s++) {
                float xf = (xp+frand()) / (float)film.xres;
                float yf = (yp+frand()) / (float)film.yres;

                vec3 orig(0,0,6);
                // vec3 dir(normalize(vec3(xf*2-1, yf*2-1, -1)));
                vec3 dir(normalize(vec3(((xofs+xf*film.xres)/fullresx)*2-1,
                                        ((yofs+yf*film.yres)/fullresy)*2-1, -1)));
                float xfn = (xofs + xf*film.xres) / fullresx;
                float yfn = (yofs + yf*film.yres) / fullresy;
                // auto kkk = cam->generate_ray(xfn,yfn, frand(),frand());
                // orig = orig + kkk.first;
                // dir = kkk.second;
                // dir = cam->generate_ray(xfn,yfn, .5,.5);
                // std::cout << xfn << "," << yfn << std::endl;

                // Ray ray(orig, dir);
                Ray ray( cam->generate_ray(xfn,yfn, frand(),frand()) );
                Isect isect;
                Spectrum L(0.0f);

                if (scene->intersect(ray, &isect)) {
                    // std::cout << isect.p << std::endl;
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


#include "lisc_gray.hpp"
#include "lisc_linalg.hpp"

Scene* evaluate_scene (Value& description) {
    auto e = Evaluator();
    e.add_set(evaluate_linalg);
    e.add_set(evaluate_gray);
    std::cout << description << std::endl;
    e.evaluate(description);
    std::cout << description << std::endl;

    Scene* scene = new Scene();
    ListAggregate* agg = new ListAggregate();
    std::shared_ptr<Primitive> p = nullptr;
    while ( (p = pop_attr<Primitive>("_prim", nullptr, description.list)) ) {
        agg->add(p.get());
    }
    scene->primitives = agg;
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
        else if (strcmp(argv[i], "-xxx") == 0) {
            xxx = atof(argv[++i]);
        }
        else {
            input_filename = argv[i];
        }
    }



    Scene* scene = nullptr;
    try {
        scene = load(input_filename);

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
