
#include "film.hpp"
#include <iostream>

float frand ()
{
    return rand() / (float)RAND_MAX;
}

int min_elem (const vec3& a)
{
    return ((a.x < a.y)
            ? ((a.x < a.z)
               ? 0
               : (a.z < a.y) ? 2 : 1)
            : ((a.y < a.z)
               ? 1
               : (a.x < a.z) ? 0 : 2));
}

void orthonormal_basis (const vec3& r, vec3* s, vec3* t)
{
    int i = min_elem(r);
    int i2 = (i+1) % 3;
    int i3 = (i+2) % 3;
    if (i3 < i2) std::swap(i2,i3);

    (*s)[i] = 0;
    (*s)[i2] = -r[i3];
    (*s)[i3] = r[i2];
    (*s) = normalize(*s);

    (*t) = cross(r, *s);
}

/// Builds transform that changes vector from world space to tangent space.
Transform build_tangent_from_world (const vec3& normal)
{
    vec3 t, b;
    orthonormal_basis(normal, &t, &b);

    glm::mat4 m(glm::vec4(t, 1.0f),
                glm::vec4(b, 1.0f),
                glm::vec4(normal, 1.0f),
                glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    return Transform(glm::transpose(m), m);
}

/**
 * u_S = (c1,c2,c3) = vector in V
 * S = {v1,v2,v3} = basis of vector space V
 * u = c1*v1 + c2*v2 + c3*v3 = S^T * u_S
 */


vec3 sample_hemisphere (const vec2& uv)
{
    float z = uv[0];
    float r = sqrtf(1.0f - z*z);
    float phi = uv[1] * M_2PI;

    return vec3(cos(phi)*r, sin(phi)*r, z);
}



class Texture
{
public:
    Spectrum sample (const vec3& pos) const;

    Spectrum R;
};


class Lambertian : public BSDF
{
public:
    Lambertian (const Spectrum& rho) : rho(rho) {}
    Spectrum rho;

    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv) const
    {
        *wi = sample_hemisphere(uv);
        return rho / (float)M_PI; // i still don't get why it's pi and not 2pi...
    }
};

BSDF* Material::get_bsdf (const vec3& p) const
{
    return new Lambertian(R);
}


#include "lisc_gray.hpp"

ListAggregate* load (const char* filename)
{
    Evaluator ev;
    LiscGray lg(&ev);
    ev.evaluate_file(filename);

    ListAggregate* list = new ListAggregate();
    for (auto p : lg.primitives) {
        list->add(p);
        GeometricPrimitive* pp = (GeometricPrimitive*)p;
    }

    return list;
}



int main (int argc, char* argv[])
{
    Film film(256,256);

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

    int spp = 2;

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
                    L = f * (float)fmax(dot(wo, normalize(vec3(-1,1,0))), 0.f) * 10.0f;
                }

                film.add_sample(xf,yf, L);
            }
        }
    }

    film.save("out.png");

    return 0;
}
