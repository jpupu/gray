#include "materials.hpp"
#include "util.hpp"

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
        return new Specular(R);
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
