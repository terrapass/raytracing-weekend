#ifndef RTWE_MATH_UTILS_H
#define RTWE_MATH_UTILS_H

#include <optional>

#include "types.h"
#include "constants.h"

namespace rtwe
{

//
// Utilities
//

std::optional<std::pair<float, float>> solveQuadraticEquation(const float a, const float b, const float c);

bool isAlmostEqual(const float value0, const float value1, const float epsilon = EPSILON);

Vector3 multiplyElements(const Vector3 & vector0, const Vector3 & vector1);

}

#endif // RTWE_MATH_UTILS_H
