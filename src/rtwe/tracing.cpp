#include "tracing.h"

namespace rtwe
{

//
// Constants
//

const Color BACKGROUND_COLOR_TOP    = Color::CYAN;
const Color BACKGROUND_COLOR_BOTTOM = Color::YELLOW;

//
// Utilities
//

Color GetMissedRayColor(const Ray & ray)
{
    return LerpColor(
        BACKGROUND_COLOR_BOTTOM,
        BACKGROUND_COLOR_TOP,
        0.5f + 0.5f*ray.Direction.normalized().y()
    );
}

}
