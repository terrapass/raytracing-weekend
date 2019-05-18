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

}

#endif // RTWE_TRACING_H
