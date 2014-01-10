#include "materials.hpp"
#include "util.hpp"

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
        if (cos_i < 0) std::swap(ei, et);

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

    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv) const
    {
        *wi = sample_hemisphere(uv);
        return rho / (float)M_PI; // i still don't get why it's pi and not 2pi...
    }
};

class Specular : public BSDF
{
public:
    Specular (const Spectrum& rho) : rho(rho) {}
    Spectrum rho;

    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv) const
    {
        *wi = vec3(-wo.x, -wo.y, wo.z);
        return rho;
    }
};

class SpecularReflection : public BSDF
{
public:
    Spectrum R;
    Fresnel* fresnel;

    SpecularReflection (const Spectrum& R, Fresnel* f)
        : R(R), fresnel(f)
    { }

    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv) const
    {
        *wi = vec3(-wo.x, -wo.y, wo.z);
        return (*fresnel)(cos_theta(wo)) * R / abs_cos_theta(*wi);
    }
};


class Diffuse : public Material
{
public:
    Diffuse (const Spectrum& R)
        : R(R)
    { }

    Spectrum R;

    virtual BSDF* get_bsdf (const vec3& p) const
    {
        return new Lambertian(R);
    }

};

class Mirror : public Material
{
public:
    Mirror (const Spectrum& R)
        : R(R)
    { }

    Spectrum R;

    virtual BSDF* get_bsdf (const vec3& p) const
    {
        return new SpecularReflection(R, new FresnelDielectric(1.0f, 1.3f));
    }

};



Material* make_diffuse (const Spectrum& reflectance)
{
	return new Diffuse(reflectance);
}
Material* make_mirror (const Spectrum& reflectance)
{
	return new Mirror(reflectance);
}
