#include "tracing.h"

#include "math_utils.h"

namespace rtwe
{

//
// Utilities
//

Color GetVerticalGradientColor(const Ray & ray, const Color & bottomColor, const Color & topColor)
{
    return LerpColor(
        bottomColor,
        topColor,
        0.5f + 0.5f*ray.Direction.normalized().y()
    );
}

static inline std::optional<float> TryRayHitSphereImpl(const Ray & ray, const Vector3 & sphereCenter, const float sphereRadius);

std::optional<RayHit> TryRayHitSphere(const Ray & ray, const Vector3 & sphereCenter, const float sphereRadius)
{
    const std::optional<float> rayHitParam = TryRayHitSphereImpl(ray, sphereCenter, sphereRadius);

    if (!rayHitParam.has_value())
        return std::nullopt;

    RayHit rayHit;
    rayHit.RayParam  = rayHitParam.value();
    rayHit.Hitpoint  = ray.GetPointAtParameter(*rayHitParam);
    rayHit.RawNormal = rayHit.Hitpoint - sphereCenter;

    return rayHit;
}

//
// Service
//

static inline std::optional<float> TryRayHitSphereImpl(const Ray & ray, const Vector3 & sphereCenter, const float sphereRadius)
{
    const Vector3 vectorToSphereCenter = sphereCenter - ray.Origin;

    const auto solutions = solveQuadraticEquation(
        ray.Direction.dot(ray.Direction),
        2.0f*(ray.Direction.dot(vectorToSphereCenter)),
        vectorToSphereCenter.dot(vectorToSphereCenter) - sphereRadius*sphereRadius
    );

    if (!solutions.has_value())
        return std::nullopt;

    return solutions->first;
}

}
