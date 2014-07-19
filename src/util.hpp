#ifndef UTIL_H__
#define UTIL_H__

#include "mymath.hpp"
#include "Transform.hpp"
#include <algorithm>
#include <random>

inline
float frand ()
{
    static std::mt19937 rng;
    return rng() / (float)0xffffffff;
    // return rand() / (float)RAND_MAX;
}

inline
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

inline
int abs_min_elem (const vec3& a)
{
    float x = fabs(a.x);
    float y = fabs(a.y);
    float z = fabs(a.z);
    return ((x < y)
            ? ((x < z)
               ? 0
               : (z < y) ? 2 : 1)
            : ((y < z)
               ? 1
               : (x < z) ? 0 : 2));
}

inline
int max_elem (const vec3& a)
{
    return ((a.x > a.y)
            ? ((a.x > a.z)
               ? 0
               : (a.z > a.y) ? 2 : 1)
            : ((a.y > a.z)
               ? 1
               : (a.x > a.z) ? 0 : 2));
}

inline
int abs_max_elem (const vec3& a)
{
    float x = fabs(a.x);
    float y = fabs(a.y);
    float z = fabs(a.z);
    return ((x > y)
            ? ((x > z)
               ? 0
               : (z > y) ? 2 : 1)
            : ((y > z)
               ? 1
               : (x > z) ? 0 : 2));
}

inline
void orthonormal_basis (const vec3& r, vec3* s, vec3* t)
{
    int i = abs_min_elem(r);
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
inline
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

inline
vec3 uniform_sample_hemisphere (const vec2& uv)
{
    float z = uv[0];
    float r = sqrtf(1.0f - z*z);
    float phi = uv[1] * M_2PI;

    return vec3(cos(phi)*r, sin(phi)*r, z);
}

inline
float uniform_hemisphere_pdf ()
{
    return 1.0f / M_2PI;
}


/** Compute cosine of angle between w and normal (0,0,1).
 * w must be unit vector.
 */
inline
float cos_theta (const vec3& w)
{
    return w.z;
}

/** Compute absolute of cosine of angle between w and normal (0,0,1).
 * w must be unit vector.
 */
inline
float abs_cos_theta (const vec3& w)
{
    return fabs(w.z);
}

/** Compute sine squared of angle between w and normal (0,0,1).
 * w must be unit vector.
 */
inline
float sin2_theta (const vec3& w)
{
    return 1 - cos_theta(w) * cos_theta(w);
}

/** Compute sine of angle between w and normal (0,0,1).
 * w must be unit vector.
 */
inline
float sin_theta (const vec3& w)
{
    return sqrtf(sin2_theta(w));
}

/** Compute tangent of angle between w and normal (0,0,1).
 * w must be unit vector.
 */
inline
float tan_theta (const vec3& w)
{
    // w_xy = (w_x, w_y, 0)
    // tan phi = |w_xy| / w_z
    return sqrtf(w.x*w.x + w.y*w.y) / w.z;
}

/** Compute cosine of angle between w projected on xy-plane and (1,0,0).
 * w must be unit vector.
 */
inline
float cos_phi (const vec3& w)
{
    // w_xy = (w_x, w_y, 0)
    // |w_xy| cos phi = w_xy . (1,0,0) = w_x
    // <=> cos phi = w_x / |w_xy| = w_x / sqrt(w_x^2 + w_y^2)

    float discr = w.x*w.x + w.y*w.y;

    if (discr > 0) {
        return w.x / sqrtf(discr);
    }
    else {
        return 0;
    }
}

/** Compute sine of angle between w projected on xy-plane and (1,0,0).
 * w must be unit vector.
 */
inline
float sin_phi (const vec3& w)
{
    // w_xy = (w_x, w_y, 0)
    // sin phi = w_xy . (0,1,0) / |w_xy| = w_y / |w_xy| = w_y / sqrt(w_x^2 + w_y^2)

    float discr = w.x*w.x + w.y*w.y;

    if (discr > 0) {
        return w.y / sqrtf(w.x*w.x + w.y*w.y);
    }
    else {
        return 1;
    }
}

/** Compute tangent of angle between w projected on xy-plane and (1,0,0).
 * w must be unit vector.
 */
inline
float tan_phi (const vec3& w)
{
    // w_xy = (w_x, w_y, 0)
    // tan phi = w_y / w_x
    return w.y / w.x;
}




 
#endif /* end of include guard: UTIL_H__ */