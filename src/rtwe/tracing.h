#ifndef RTWE_TRACING_H
#define RTWE_TRACING_H

#include "types.h"
#include "Color.h"
#include "Ray.h"

namespace rtwe
{

//
// Utilities
//

Color GetMissedRayColor(const Ray & ray);

bool DoesRayHitSphere(const Ray & ray, const Vector3 & sphereCenter, const float sphereRadius);

}

#endif // RTWE_TRACING_H
