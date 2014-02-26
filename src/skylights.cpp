#include "gray.hpp"
#include "lisc.hpp"

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



static
std::vector<std::shared_ptr<Skylight>> sky_pool;

void evaluate_skylight (Value& val, List& args)
{
    Skylight* sky;
    auto name = *pop<std::string>(args);
    if (name == "solid") {
        auto R = *pop_attr<Spectrum>("R", args);
        sky = new SolidSkylight(R);
    }
    else {
        throw std::runtime_error("invalid skylight name "+name);
    }

    std::shared_ptr<Skylight> sh(sky);
    sky_pool.push_back(sh);
    val.reset({"_skylight", sh});
}
