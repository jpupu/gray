
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



int main (int argc, char* argv[])
{
    Film film(512,512);

    Scene* scene = nullptr;
    try {
        scene = load("test1.scene");
    }
    catch (std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }

    int spp = 10;

    for (int yp = 0; yp < film.yres; yp++) {
        for (int xp = 0; xp < film.xres; xp++) {
            for (int s = 0; s < spp; s++) {
                float xf = (xp+frand()) / (float)film.xres;
                float yf = (yp+frand()) / (float)film.yres;

                vec3 orig(0,0,3);
                vec3 dir(normalize(vec3(xf*2-1, yf*2-1, -1)));

                Ray ray(orig, dir);
                Isect isect;
                Spectrum L(0.0f);

                if (scene->intersect(ray, &isect)) {
                    BSDF* bsdf = isect.mat->get_bsdf(isect.p);
                    Transform tangent_from_world = build_tangent_from_world(isect.n);
                    vec3 wo_t = tangent_from_world.vector(-ray.d);
                    vec3 wi_t;
                    Spectrum f = bsdf->sample(wo_t, &wi_t, glm::vec2(frand(),frand()));
                    delete bsdf;
                    vec3 wo = inverse(tangent_from_world).vector(wi_t);
                    L = f * (float)fmax(dot(wo, normalize(vec3(-10,7,3))), 0.f) * 10.0f;
                }
                else L = Spectrum(0,1,0);

                film.add_sample(xf,yf, L);
            }
        }
    }

    film.save("out.png");

    return 0;
}
