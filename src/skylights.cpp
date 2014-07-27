#include "gray.hpp"
#include "lisc.hpp"
extern "C" {
#include "rgbe.h"
}

class SolidSkylight : public Skylight
{
public:
    Spectrum R;

    SolidSkylight (const Spectrum& R)
        : R(R)
    { }

    Spectrum sample (const vec3& dir) const
    {
        return R;
    }
};

class CosineSkylight : public Skylight
{
public:
    Spectrum R;

    CosineSkylight (const Spectrum& R)
        : R(R)
    { }

    Spectrum sample (const vec3& dir) const
    {
        return R * std::max(0.0f, dir.y*dir.y*dir.y);
    }
};

class ColorSkylight : public Skylight
{
public:
    ColorSkylight ()
    { }

    Spectrum sample (const vec3& dir) const
    {
        float theta = acosf(abs_cos_theta(dir));
        float phi = acosf(cos_phi(dir));
        bool vgrid = fmod(theta+2*M_PI, M_PI/16) > .01;
        bool hgrid = fmod(phi+2*M_PI, M_PI/8) > .01;
        return (dir + vec3(1.0f)) * .5f * float(vgrid*hgrid);
    }
};

class DebevecSkylight : public Skylight
{
public:
    std::unique_ptr<Spectrum[]> R;
    int resx, resy;

    DebevecSkylight (const std::string& filename)
    {
        load_hdr(filename);
    }

    Spectrum sample (const vec3& dir) const
    {
        //  If for a direction vector in the world (Dx, Dy, Dz),
        // the corresponding (u,v) coordinate in the light probe image is 
        // (Dx*r,Dy*r) where r=(1/pi)*acos(Dz)/sqrt(Dx^2 + Dy^2).

        double r = (1/M_PI) * acosf(dir.z) / sqrtf(dir.x*dir.x + dir.y*dir.y);
        float u = dir.x * r;
        float v = -dir.y * r;
        int uu = resx * (u + 1) / 2;
        int vv = resy * (v + 1) / 2;
        uu = std::max(0, std::min(resx-1, uu));
        vv = std::max(0, std::min(resy-1, vv));
        return R[uu + vv*resx];
    }

    void load_hdr (const std::string& filename)
    {
        FILE* fp = fopen(filename.c_str(), "rb");
        rgbe_header_info info;
        if (RGBE_ReadHeader(fp, &resx, &resy, &info)) {
            fclose(fp);
            throw std::runtime_error("Error reading HDR file header.");
        }

        R.reset(new Spectrum[resx*resy]);

        if (RGBE_ReadPixels_RLE(fp, (float*)R.get(), resx, resy)) {
            fclose(fp);
            throw std::runtime_error("Error reading HDR file pixels.");
        }
    }
};




void evaluate_skylight (Value& val, List& args)
{
    Skylight* sky;
    auto name = *pop<std::string>(args);
    if (name == "solid") {
        auto R = *pop<Spectrum>(args);
        sky = new SolidSkylight(R);
    }
    else if (name == "cosine") {
        auto R = *pop<Spectrum>(args);
        sky = new CosineSkylight(R);
    }
    else if (name == "color") {
        sky = new ColorSkylight();
    }
    else if (name == "probe") {
        auto file = *pop<std::string>(args);
        sky = new DebevecSkylight(file);
    }
    else {
        throw std::runtime_error("invalid skylight name "+name);
    }

    std::shared_ptr<Skylight> sh(sky);
    val.reset({"_skylight", sh});
}
