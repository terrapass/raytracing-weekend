#ifndef RTWE_MATH_UTILS_H
#define RTWE_MATH_UTILS_H

#include <optional>

namespace rtwe
{

//
// Utilities
//

std::optional<std::pair<float, float>> solveQuadraticEquation(const float a, const float b, const float c);

}

#endif // RTWE_MATH_UTILS_H
