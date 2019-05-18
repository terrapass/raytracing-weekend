#include "tracing.h"

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

}
