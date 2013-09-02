
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

ListAggregate* load (const char* filename)
{
    Evaluator ev;
    LiscLinAlg* la = new LiscLinAlg(&ev);
    LiscGray lg(&ev, la);
    ev.evaluate_file(filename);

    ListAggregate* list = new ListAggregate();
    for (auto p : lg.primitives) {
        // GeometricPrimitive* pp = (GeometricPrimitive*)p;
        // pp->mat = new Mirror(vec3(1,1,1));
        list->add(p);
    }

    return list;
}



int main (int argc, char* argv[])
{
    Film film(512,512);

    // Material mat1 = { Spectrum(1.0f, 0.0f, .5f) };
    // Material mat2 = { Spectrum(.8f, 1.0f, .5f) };
    // Shape* sp1 = new Sphere();
    // GeometricPrimitive prim1;
    // prim1.mat = &mat1;
    // prim1.shape = sp1;
    // prim1.world_from_prim = Transform::rotate(90, vec3(0,1,0));
    // GeometricPrimitive prim2;
    // prim2.mat = &mat2;
    // prim2.shape = sp1;
    // prim2.world_from_prim = Transform::translate(vec3(1.4,1,-1));
    // ListAggregate list;
    // list.add(&prim1);
    // list.add(&prim2);

    ListAggregate* list = nullptr;
    try {
        list = load("test1.scene");
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

                if (list->intersect(ray, &isect)) {
                    BSDF* bsdf = isect.mat->get_bsdf(isect.p);
                    Transform tangent_from_world = build_tangent_from_world(isect.n);
                    vec3 wo_t = tangent_from_world.vector(-ray.d);
                    vec3 wi_t;
                    Spectrum f = bsdf->sample(wo_t, &wi_t, glm::vec2(frand(),frand()));
                    delete bsdf;
                    vec3 wo = inverse(tangent_from_world).vector(wi_t);
                    L = f * (float)fmax(dot(wo, normalize(vec3(0,0,1))), 0.f) * 10.0f;
                }
                else L = Spectrum(0,1,0);

                film.add_sample(xf,yf, L);
            }
        }
    }

    film.save("out.png");

    return 0;
}
