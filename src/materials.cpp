#include "util.hpp"
#include "lisc_gray.hpp"
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


class FresnelConductor : public Fresnel
{
public:
    /// index of refraction
    Spectrum eta;
    /// absorbtion coefficient
    Spectrum k;

    FresnelConductor (Spectrum eta, Spectrum k)
        : eta(eta), k(k)
    { }

    Spectrum evaluate (float cos_i) const
    {
        // reflectance for paraller polarized light
        // r||^2 = (eta^2 + k^2) cos^2 - 2 eta cos + 1
        //         ------------------------------------
        //         (eta^2 + k^2) cos^2 + 2 eta cos + 1
        //
        // reflectance for perpendicular polarized light
        // r|-^2 = (eta^2 + k^2) - 2 eta cos + cos^2
        //         ------------------------------------
        //         (eta^2 + k^2) + 2 eta cos + cos^2
        //
        // For unpolarized light,
        // Fr = r||^2 + r|-^2
        //      --------------
        //            2

        // float e2k2 = eta*eta + k*k;
        // float eta2cos = 2 * eta * cos_i;
        // float cos2 = cos_i * cos_i;

        // float rpar2 = (e2k2*cos2 - eta2cos + 1) / (e2k2*cos2 + eta2cos + 1);
        // float rper2 = (e2k2 - eta2cos + cos2) / (e2k2 + eta2cos + cos2);

        // If the ray is coming from the inside, it's probably a clipping bug.
        if (cos_i < 0) return Spectrum(0.0f);

        Spectrum e2k2 = eta*eta + k*k;
        Spectrum eta2cos = 2.0f * eta * cos_i;
        float cos2 = cos_i * cos_i;

        Spectrum rpar2 = (e2k2*cos2 - eta2cos + Spectrum(1)) / (e2k2*cos2 + eta2cos + Spectrum(1));
        Spectrum rper2 = (e2k2 - eta2cos + cos2) / (e2k2 + eta2cos + cos2);

        // std::cout << "eta " << eta << " cos_i " << cos_i << std::endl;
        // std::cout << "rper2 " << rper2[0] << std::endl;
        // std::cout << "rpar2 " << rpar2[0] << std::endl;
        return (rpar2 + rper2) * 0.5f;
    }
};

class FresnelOne : public Fresnel
{
public:
    FresnelOne ()
    { }

    Spectrum evaluate (float cos_i) const
    {
        return Spectrum(1.0f);
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
        return rho / abs_cos_theta(*wi);
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
            return Spectrum(0);
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


class OrenNayar : public BSDF
{
public:
    OrenNayar (const Spectrum& rho, const Spectrum& sigma)
        : rho(rho)
    {
        float sigma2 = sigma.r * sigma.r;
        A = 1 - sigma2 / (2 * (sigma2 + 0.33));
        B = 0.45*sigma2 / (sigma2 + 0.09);
    }
    /// reflectance
    Spectrum rho;
    float A, B;

    virtual Spectrum sample (const vec3& wo, vec3* wi, const vec2& uv, float* pdf) const
    {
        // a = max(theta_i, theta_o)
        // b = min(theta_i, theta_o)
        //
        // fr(w_i, w_o) = rho/pi (A + B max(0,cos(phi_i - phi_o)) sin a tan b)
        // 
        //    [ cos(x-y) = cos x cos y + sin x sin y ]
        //

        if (wo.z <= 0) return Spectrum(0);

        *wi = uniform_sample_hemisphere(uv);
        *pdf = uniform_hemisphere_pdf();
        
        // cos X < cos Y  <==> X > Y
        bool igto = cos_theta(*wi) < cos_theta(wo);
        float sin_a = igto ? sin_theta(*wi) : sin_theta(wo);
        float tan_b = igto ? tan_theta(wo) : tan_theta(*wi);

        float c = cos_phi(*wi)*cos_phi(wo) + sin_phi(*wi)*sin_phi(wo);
        // float c = cos_phi(*wi)*cos_phi(wo)*1 + 1*sin_phi(*wi)*sin_phi(wo);
        // c = c*sin_a*tan_b;

        float term = (A + B * std::max(0.0f, c) * sin_a * tan_b);

        debug::add("wo", wo);
        debug::add("wi", *wi);
        debug::add("c", c);
        debug::add("sin_a", sin_a);
        debug::add("tan_b", tan_b);

        return term * rho / (float)M_PI;
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

void ttt (int argc, char* argv[])
{
    FresnelConductor fr(Spectrum(atof(argv[1])), Spectrum(atof(argv[2])));
    std::cout << std::endl << 0 << std::endl;
    std::cout << fr.evaluate(cos(0.0f/180*M_PI))[0] << std::endl;
    std::cout << std::endl << 30 << std::endl;
    std::cout << fr.evaluate(cos(30.0f/180*M_PI))[0] << std::endl;
    std::cout << std::endl << 60 << std::endl;
    std::cout << fr.evaluate(cos(60.0f/180*M_PI))[0] << std::endl;
    std::cout << std::endl << 90 << std::endl;
    std::cout << fr.evaluate(cos(90.0f/180*M_PI))[0] << std::endl;
}

class Texture
{
public:
    virtual Spectrum sample (const vec2& uv, const vec3& p) const = 0;
};

class SolidColor : public Texture
{
public:
    SolidColor (const Spectrum& a)
        : A(a)
    { }

    Spectrum sample (const vec2& uv, const vec3& p) const
    {
        return A;
    }

private:
    Spectrum A;
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

class Grid3D : public Texture
{
public:
    Grid3D (const Spectrum& a, const Spectrum& b, float width = .05)
        : A(a), B(b), width(width)
    { }

    Spectrum sample (const vec2& uv, const vec3& p) const
    {
        return ((p.x - floor(p.x) < width) ||
                (p.y - floor(p.y) < width) ||
                (p.z - floor(p.z) < width)) ? B : A;
    }

private:
    Spectrum A, B;
    float width;

};

class Diffuse : public Material
{
public:
    Diffuse (const shared_ptr<Texture>& R)
        : R(R)
    { }

    shared_ptr<Texture> R;
    Transform xform;

    virtual unique_ptr<BSDF> get_bsdf (const vec3& p, const vec2& u) const
    {
        Spectrum r = R->sample(vec2(0,0), xform.point(p));
        return unique_ptr<BSDF>(new Lambertian(r));
    }

};

class Diffuse2 : public Material
{
public:
    Diffuse2 (const shared_ptr<Texture>& R, const std::shared_ptr<Texture>& S)
        : R(R), S(S)
    { }

    shared_ptr<Texture> R;
    shared_ptr<Texture> S;
    Transform xform;

    virtual unique_ptr<BSDF> get_bsdf (const vec3& p, const vec2& u) const
    {
        Spectrum r = R->sample(vec2(0,0), xform.point(p));
        Spectrum s = S->sample(vec2(0,0), xform.point(p));
        return unique_ptr<BSDF>(new OrenNayar(r, s));
    }

};

class Mirror : public Material
{
public:
    Mirror (const Spectrum& R)
        : R(R)
    { }

    Spectrum R;

    virtual unique_ptr<BSDF> get_bsdf (const vec3& p, const vec2& u) const
    {
        return unique_ptr<BSDF>(new SpecularReflection(R, make_shared<FresnelOne>()));
    }

};

class Metal : public Material
{
public:
    Metal (const Spectrum& n, const Spectrum& k)
        : n(n), k(k)
    { }

    Spectrum n, k;

    virtual unique_ptr<BSDF> get_bsdf (const vec3& p, const vec2& u) const
    {
        // return unique_ptr<BSDF>(new SpecularReflection(R, make_shared<FresnelConductor>(0.05f, 3.131f)));
        // 650, 
        return unique_ptr<BSDF>(new SpecularReflection(Spectrum(1), make_shared<FresnelConductor>(n,k)));
                                // Spectrum(0.21845, 1.16576, 1.031265), Spectrum(3.6370, 3.0957, 2.3896))));
                                // Spectrum(0.2378, 1.0066269, 1.31346), Spectrum(3.6264, 2.5823, 2.1309))));
        // return unique_ptr<BSDF>(new SpecularReflection(R, make_shared<FresnelDielectric>(1.0f, 1.5f)));
        // return unique_ptr<BSDF>(new Specular(R));
    }

};

class Glass : public Material
{
public:
    Glass (const Spectrum& R)
        : R(R)
    { }

    Spectrum R;

    virtual unique_ptr<BSDF> get_bsdf (const vec3& p, const vec2& u) const
    {
        // These should be scaled by 2, because p == 1/2.
        // But we can't scale a BSDF.
        // Instead we probably should return a combination BSDF.
        // Well, I guess we can scale the R.
        // if (frand() < 0.5f) {
        if (u.x < 0.5f) {
            return unique_ptr<BSDF>(new SpecularReflection(2.0f*R, make_shared<FresnelDielectric>(1.0f, 1.3f)));
        }
        else {
            return unique_ptr<BSDF>(new SpecularTransmission(2.0f*R, make_shared<FresnelDielectric>(1.0f, 1.3f)));
        }

        // return unique_ptr<BSDF>(new SpecularTransmission(R, make_shared<FresnelDielectric>(1.0f, 1.3f)));
    }

};



bool evaluate_texture (Value& val, const std::string& name, List& args)
{
    shared_ptr<Texture> T;
    if (name == "checker") {
        Spectrum a = *pop<Spectrum>(args);
        Spectrum b = *pop<Spectrum>(args);
        T = make_shared<Checkers3D>(a, b);
    }
    else if (name == "grid") {
        float w = *pop<double>(args);
        Spectrum a = *pop<Spectrum>(args);
        Spectrum b = *pop<Spectrum>(args);
        T = make_shared<Grid3D>(a, b, w);
    }
    else if (name == "solid") {
        Spectrum a = *pop<Spectrum>(args);
        T = make_shared<SolidColor>(a);
    }
    else {
        return false;
    }

    val.reset(T);
    return true;
}


shared_ptr<Texture> pop_texture (List& args)
{
    if (args.front().is<Texture>()) {
        return pop<Texture>(args);
    }
    else if (args.front().is<vec3>()) {
        return make_shared<SolidColor>(Spectrum(*pop<vec3>(args)));
    }
    throw std::runtime_error("failed to extract texture or color");
}



bool evaluate_material (Value& val, const std::string& name, List& args)
{
    std::shared_ptr<Material> M;
    if (name == "diffuse") {
        auto MM = make_shared<Diffuse>(pop_texture(args));
        MM->xform = pop_transforms(args);
        M = MM;
    }
    else if (name == "diffuse2") {
        auto rho = pop_texture(args);
        auto sigma = pop_texture(args);
        auto MM = make_shared<Diffuse2>(rho, sigma);
        MM->xform = pop_transforms(args);
        M = MM;
    }
    else if (name == "mirror") {
        Spectrum R = *pop<Spectrum>(args);
        M = make_shared<Mirror>(R);
    }
    else if (name == "metal") {
        Spectrum n = *pop<Spectrum>(args);
        Spectrum k = *pop<Spectrum>(args);
        M = make_shared<Metal>(n, k);
    }
    else if (name == "glass") {
        Spectrum R = *pop<Spectrum>(args);
        M = make_shared<Glass>(R);
    }
    else {
        return false;
    }

    val.reset(M);
    return true;
}


