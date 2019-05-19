#ifndef RTWE_TRACING_H
#define RTWE_TRACING_H

#include "types.h"
#include "Color.h"
#include "Ray.h"

namespace rtwe
{

//
// Interface types
//

struct RayHit final
{
    float   RayParam;
    Vector3 Hitpoint;
    Vector3 RawNormal;
};

//
// Utilities
//

Color GetMissedRayColor(const Ray & ray);

std::optional<RayHit> TryRayHitSphere(const Ray & ray, const Vector3 & sphereCenter, const float sphereRadius);

}

#endif // RTWE_TRACING_H
