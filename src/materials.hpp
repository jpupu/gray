#ifndef MATERIALS_H__
#define MATERIALS_H__

#include "gray.hpp"

Material* make_diffuse (const Spectrum& reflectance);
Material* make_mirror (const Spectrum& reflectance);


 
#endif /* end of include guard: MATERIALS_H__ */