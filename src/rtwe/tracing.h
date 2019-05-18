#ifndef RTWE_TRACING_H
#define RTWE_TRACING_H

#include "types.h"
#include "Color.h"
#include "Ray.h"

namespace rtwe
{

//
// Constants
//

extern const Color BACKGROUND_COLOR_TOP;
extern const Color BACKGROUND_COLOR_BOTTOM;

//
// Utilities
//

Color GetMissedRayColor(const Ray & ray);

}

#endif // RTWE_TRACING_H
