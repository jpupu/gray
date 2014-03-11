#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

using glm::vec3;

struct Transform {
    glm::mat4 m, m_inv;

    Transform () : m(1), m_inv(1) { }
    Transform (const glm::mat4& m) : m(m), m_inv(glm::inverse(m)) { }
    Transform (const glm::mat4& m, const glm::mat4& m_inv) : m(m), m_inv(m_inv) { }

    vec3 vector (const vec3& v) const;
    vec3 point (const vec3& v) const;
    vec3 normal (const vec3& v) const;

    static Transform rotate (float degrees, const vec3& axis);
    static Transform scale (const vec3& v);
    static Transform translate (const vec3& v);
    static Transform look_at (const vec3& eye, const vec3& center, const vec3& up);

};

inline Transform inverse (const Transform& t)
{
    return Transform(t.m_inv, t.m);
}



inline vec3 Transform::vector (const vec3& v) const
{
    return vec3(m * glm::vec4(v, 0));
}
inline vec3 Transform::point (const vec3& v) const
{
    return vec3(m * glm::vec4(v, 1));
}
inline vec3 Transform::normal (const vec3& v) const
{
    return vec3(glm::transpose(m_inv) * glm::vec4(v, 0));
}


inline Transform Transform::rotate (float degrees, const vec3& axis)
{
    return Transform(glm::rotate(degrees, axis), glm::rotate(-degrees, axis));
}

inline Transform Transform::scale (const vec3& v)
{
    return Transform(glm::scale(v), glm::scale(1.0f/v));
}

inline Transform Transform::translate (const vec3& v)
{
    return Transform(glm::translate(v), glm::translate(-v));
}

inline Transform Transform::look_at (const vec3& eye, const vec3& center, const vec3& up)
{
    return Transform(glm::lookAt(eye, center, up));
}

inline Transform operator* (const Transform& a, const Transform& b)
{
    return Transform(a.m * b.m, b.m_inv * a.m_inv);
}

#endif /* TRANSFORM_HPP */
