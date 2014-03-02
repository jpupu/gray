#ifndef MYMATH_HPP
#define MYMATH_HPP
#include <cmath>
#include <glm/glm.hpp>
#include <ostream>

using glm::vec2;
using glm::vec3;

using glm::dot;
using glm::cross;
using glm::normalize;
using glm::clamp;

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif
#define M_2PI (2*M_PI)


inline
std::ostream& operator<< (std::ostream& os, const glm::vec3& v)
{
    os << "<" << v.x << ", " << v.y << ", " << v.z << ">";
    return os;
}




#endif /* MYMATH_HPP */
