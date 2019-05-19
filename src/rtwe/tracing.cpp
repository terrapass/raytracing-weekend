#include "tracing.h"

#include "math_utils.h"

namespace rtwe
{

//
// Utilities
//

Color GetMissedRayColor(const Ray & ray)
{
    static const Color BACKGROUND_COLOR_BOTTOM = Color::YELLOW;
    static const Color BACKGROUND_COLOR_TOP    = Color::CYAN;

    return LerpColor(
        BACKGROUND_COLOR_BOTTOM,
        BACKGROUND_COLOR_TOP,
        0.5f + 0.5f*ray.Direction.normalized().y()
    );
}

bool DoesRayHitSphere(const Ray & ray, const Vector3 & sphereCenter, const float sphereRadius)
{
    const Vector3 vectorFromShereCenter = ray.Origin - sphereCenter;

    const auto solutions = solveQuadraticEquation(
        ray.Direction.dot(ray.Direction),
        2.0f*(ray.Direction.dot(vectorFromShereCenter)),
        vectorFromShereCenter.dot(vectorFromShereCenter) - sphereRadius*sphereRadius
    );

    return solutions.has_value();
}

}
