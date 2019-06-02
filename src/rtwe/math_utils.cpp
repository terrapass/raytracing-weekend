#include "math_utils.h"

#include <cmath>

#include "constants.h"

namespace rtwe
{

std::optional<std::pair<float, float>> solveQuadraticEquation(const float a, const float b, const float c)
{
    const float D = b*b - 4*a*c;

    if (D < 0.0f)
        return {};

    const float sqrtD   = std::sqrt(D);

    return std::make_pair(
        (b - sqrtD)/(2*a),
        (b + sqrtD)/(2*a)
    );
}

bool isAlmostEqual(const float value0, const float value1, const float epsilon)
{
    return std::abs(value1 - value0) < EPSILON;
}

Vector3 multiplyElements(const Vector3 & vector0, const Vector3 & vector1)
{
    return Vector3(
        vector0.x() * vector1.x(),
        vector0.y() * vector1.y(),
        vector0.z() * vector1.z()
    );
}

}
