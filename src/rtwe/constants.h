#ifndef RTWE_CONSTANTS_H
#define RTWE_CONSTANTS_H

#include <limits>

namespace rtwe
{

constexpr float EPSILON = std::numeric_limits<float>::epsilon();

#ifdef NDEBUG
#define RTWE_RELEASE
constexpr bool IS_RELEASE = true;
#else // NDEBUG
#define RTWE_DEBUG
constexpr bool IS_RELEASE = false;
#endif // NDEBUG

constexpr bool IS_DEBUG = !IS_RELEASE;

}

#endif
