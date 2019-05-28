#ifndef RTWE_CONSTANTS_H
#define RTWE_CONSTANTS_H

#include <limits>

#include "Color.h"

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

const Color MISSED_RAY_COLOR(0.7f, 0.7f, 0.9f);

}

#endif
