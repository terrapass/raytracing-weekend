#include "math_utils.h"

#include <cmath>

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

}
