#include "util.hpp"
#include "lisc.hpp"
#include "gray.hpp"
#include <memory>
using std::unique_ptr;
using std::make_shared;

/** Calculate dielectric fresnel reflectance F_r. Note that each wavelength
 * in the spectrum may have its own index of refraction.
 *
 * @param cosi      cosine theta_i, i.e. cos of angle between normal and incident direction
 * @param cost      cosine theta_t, i.e. cos of angle between normal and transmitted direction
 * @param etai      index of refraction for the incident media
 * @param etat      index of refraction for the transmitted media
 * @return Fresnel reflectance F_r
 */
Spectrum fresnel_dielectric (float cosi, float cost,
                             const Spectrum& etai, const Spectrum& etat)
{
    // parallel polarized light
    Spectrum rpar = (etat * cosi - etai * cost) / (etat * cosi + etai * cost);
    // perpendicular polarized light
    Spectrum rper = (etai * cosi - etat * cost) / (etai * cosi + etat * cost);
    // Assuming unpolarized light
    return (rpar*rpar + rper*rper) / 2.0f;
}

/** Calculate dielectric fresnel reflectance F_r.
 *
 * @param cosi      cosine theta_i, i.e. cos of angle between normal and incident direction
 * @param cost      cosine theta_t, i.e. cos of angle between normal and transmitted direction
 * @param etai      index of refraction for the incident media
 * @param etat      index of refraction for the transmitted media
 * @return Fresnel reflectance F_r
 */
float fresnel_dielectric (float cosi, float cost,
                          float etai, float etat)
{
    // parallel polarized light
    float rpar = (etat * cosi - etai * cost) / (etat * cosi + etai * cost);
    // perpendicular polarized light
    float rper = (etai * cosi - etat * cost) / (etai * cosi + etat * cost);
    // Assuming unpolarized light
    return (rpar*rpar + rper*rper) / 2;
}

class Fresnel
{
public:
    /// @param cosine of incident angle
    /// @return Fresnel reflectance
    virtual Spectrum evaluate (float cosi) const = 0;

    Spectrum operator() (float cosi) const { return evaluate(cosi); }
};

class FresnelDielectric : public Fresnel
{
public:
    float eta_i;
    float eta_t;

    FresnelDielectric (float eta_i, float eta_t)
        : eta_i(eta_i), eta_t(eta_t)
    { }

    Spectrum evaluate (float cos_i) const
    {
        float ei = eta_i;
        float et = eta_t;
        // swap indices if exiting
        // (incident direction is on the other side than normal)
        if (cos_i < 0) { std::swap(ei, et); cos_i = -cos_i; }
        
        // Snell's law: eta_i * sin_i = eta_t * sin_t
        // Pythagoras: c^2 = sin_i^2 + cos_i^2,  c==1 since w_i is normalized
        float sin_i = sqrtf( 1 - cos_i*cos_i );
        float sin_t = sin_i * ei/et;

        // Total internal reflection
        if (sin_t >= 1.0) {
            return Spectrum(1.0f);
        }

        float cos_t = sqrtf( 1 - sin_t*sin_t );
        return Spectrum(fresnel_dielectric(cos_i, cos_t, ei, et));
    }
};

class Lambertian : public BSDF
{
public:
    Lambertian (const Spectrum& rho) : rho(rho) {}
    /// reflectance
    Spectrum rho;

    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv, float* pdf) const
    {
        *wi = uniform_sample_hemisphere(uv);
        *pdf = uniform_hemisphere_pdf();
        return rho / (float)M_PI;
    }
};

class Specular : public BSDF
{
public:
    Specular (const Spectrum& rho) : rho(rho) {}
    Spectrum rho;

    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv, float* pdf) const
    {
        *wi = vec3(-wo.x, -wo.y, wo.z);
        *pdf = 1;
        return rho;
    }
};

class SpecularReflection : public BSDF
{
public:
    Spectrum R;
    // TODO: shared_ptr might be a bit overkill here...
    shared_ptr<Fresnel> fresnel;

    SpecularReflection (const Spectrum& R, const shared_ptr<Fresnel>& f)
        : R(R), fresnel(f)
    { }

    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv, float* pdf) const
    {
        *wi = vec3(-wo.x, -wo.y, wo.z);
        *pdf = 1;
        return (*fresnel)(cos_theta(wo)) * R / abs_cos_theta(*wi);
    }
};

class SpecularTransmission : public BSDF
{
public:
    // transmission scale factor
    Spectrum T;
    shared_ptr<FresnelDielectric> fresnel;

    SpecularTransmission (const Spectrum& T, shared_ptr<FresnelDielectric> f)
        : T(T), fresnel(f)
    { }

    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv, float* pdf) const
    {
        *pdf = 1;

        float cos_o = cos_theta(wo);
        float eo = fresnel->eta_i;
        float ei = fresnel->eta_t;
        // swap indices if exiting instead of entering media
        bool entering = (cos_o >= 0);
        if (!entering) { std::swap(eo, ei); }

        float sin2_o = sin2_theta(wo);
        // Snell's law: eta_o * sin_o = eta_i * sin_i
        //              sin_o / sin_i = eta_i / eta_o
        //              sin2_i / sin2_o = (eta_o / eta_i)^2
        //              sin2_i = (eta_o / eta_i)^2 * sin2_o
        float eta = eo / ei;
        float sin2_i = eta*eta * sin2_o;

        // Check for total internal reflection
        if (sin2_i >= 1.0) {
            return Spectrum(0.0f);
        }

        // Trigonometric identity: sin2 x + cos2 x = 1
        float cos_i = sqrtf( 1 - sin2_i );

        // Now we have wo.z.
        // To get x and y, we mirror around the point to get unbent
        // transmission vector wu.
        //
        //      wu = -wo
        //
        // Project those to the XY plane:
        //
        //      wu' = -wo'  (= -wo.xy)
        //
        // Now to get wi.xy we calculate projection wi'
        // by scaling wu' appropriately (see pbrt figure 8.11):
        //
        //       wi' = wu' * sin_i / sin_o
        //           = wu' * eta_o / eta_i    (from Snell's law)

        (*wi).x = eta * -wo.x;
        (*wi).y = eta * -wo.y;
        (*wi).z = entering ? -cos_i : cos_i;

        return (eta*eta) * (Spectrum(1) - (*fresnel)(cos_i)) * T / abs_cos_theta(*wi);
    }
};

// void ttt()
// {
//     FresnelDielectric fr(1.0, 1.3);
//     SpecularTransmission sp(Spectrum(1.0f), shared_ptr<FresnelDielectric>(&fr));
//     vec3 wo, wi;
//     Spectrum l;
//     wo = normalize(vec3(1,0,1));
//     l = sp.sample(wo, &wi, vec2(0,0));
//     printf("enter:\n");
//     printf("wo %f %f %f\n", wo.x, wo.y, wo.z);
//     printf("wi %f %f %f\n", wi.x, wi.y, wi.z);
//     printf("l = %f %f %f\n", l.x,l.y,l.z);

//     wo = normalize(vec3(1,0,-1));
//     l = sp.sample(wo, &wi, vec2(0,0));
//     printf("exit:\n");
//     printf("wo %f %f %f\n", wo.x, wo.y, wo.z);
//     printf("wi %f %f %f\n", wi.x, wi.y, wi.z);
//     printf("l = %f %f %f\n", l.x,l.y,l.z);
// }


class Texture
{
public:
    virtual Spectrum sample (const vec2& uv, const vec3& p) const = 0;
};

class Checkers3D : public Texture
{
public:
    Checkers3D (const Spectrum& a, const Spectrum& b)
        : A(a), B(b)
    { }

    Spectrum sample (const vec2& uv, const vec3& p) const
    {
        vec3 pp = p * 2.0f - vec3(1000,1000,1000);
        return (((int)floor(pp.x) ^ (int)floor(pp.y) ^ (int)floor(pp.z)) & 1) ? A : B;
    }

private:
    Spectrum A, B;
};

class Diffuse : public Material
{
public:
    Diffuse (const Spectrum& R)
        : R(R)
    { }

    Spectrum R;

    virtual unique_ptr<BSDF> get_bsdf (const vec3& p) const
    {
        Spectrum r = Checkers3D(R, Spectrum(1)).sample(vec2(0,0), p);
        return unique_ptr<BSDF>(new Lambertian(r));
    }

};

class Mirror : public Material
{
public:
    Mirror (const Spectrum& R)
        : R(R)
    { }

    Spectrum R;

    virtual unique_ptr<BSDF> get_bsdf (const vec3& p) const
    {
        return unique_ptr<BSDF>(new SpecularReflection(R, make_shared<FresnelDielectric>(1.0f, 1.3f)));
    }

};

class Glass : public Material
{
public:
    Glass (const Spectrum& R)
        : R(R)
    { }

    Spectrum R;

    virtual unique_ptr<BSDF> get_bsdf (const vec3& p) const
    {
        // These should be scaled by 2, because p == 1/2.
        // But we can't scale a BSDF.
        // Instead we probably should return a combination BSDF.
        if (frand() < 0.5f) {
            return unique_ptr<BSDF>(new SpecularReflection(R, make_shared<FresnelDielectric>(1.0f, 1.3f)));
        }
        else {
            return unique_ptr<BSDF>(new SpecularTransmission(R, make_shared<FresnelDielectric>(1.0f, 1.3f)));
        }
    }

};




void evaluate_material (Value& val, List& args)
{
    Material* M;
    auto name = *pop<std::string>(args);
    if (name == "diffuse") {
        Spectrum R = *pop_attr<Spectrum>("R", args);
        M = new Diffuse(R);
    }
    else if (name == "mirror") {
        Spectrum R = *pop_attr<Spectrum>("R", args);
        M = new Mirror(R);
    }
    else if (name == "glass") {
        Spectrum R = *pop_attr<Spectrum>("R", args);
        M = new Glass(R);
    }
    else {
        throw std::runtime_error("invalid material name");
    }

    std::shared_ptr<Material> sh(M);
    val.reset({"_material", sh});
}


