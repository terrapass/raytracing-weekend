#ifndef RTWE_TRACING_H
#define RTWE_TRACING_H

#include <memory>
#include <functional>

#include "types.h"
#include "Color.h"
#include "Ray.h"

namespace rtwe
{

//
// Forward declarations
//

struct IRayTarget;

//
// Interface types
//

struct Material final
{
    Color Albedo;
};

struct Body final
{
    std::shared_ptr<IRayTarget> RayTarget;
    Material                    Material;
};

struct RayHit final
{
    float   RayParam;
    Vector3 Hitpoint;
    Vector3 RawNormal;
};

using RayMissFunction = std::function<Color(Ray)>;

//
// Utilities
//

std::optional<Color> TryTraceRay(const std::vector<Body> & bodies, const Ray & ray);

inline Color TraceRay(const std::vector<Body> & bodies, const Ray & ray, const RayMissFunction & rayMissFunction);

inline Color TraceRayWithDefaultColor(const std::vector<Body> & bodies, const Ray & ray, const Color & defaultColor);

Color GetVerticalGradientColor(const Ray & ray, const Color & bottomColor, const Color & topColor);

std::optional<RayHit> TryRayHitSphere(const Ray & ray, const Vector3 & sphereCenter, const float sphereRadius);

std::optional<RayHit> TryRayHitPlane(const Ray & ray, const Vector3 & planePoint, const Vector3 & planeNormal);

//
// Utilities
//

inline Color TraceRay(const std::vector<Body> & bodies, const Ray & ray, const RayMissFunction & rayMissFunction)
{
    std::optional<Color> result = TryTraceRay(bodies, ray);
    if (result.has_value())
        return std::move(*result);

    return rayMissFunction(ray);
}

inline Color TraceRayWithDefaultColor(const std::vector<Body> & bodies, const Ray & ray, const Color & defaultColor)
{
    const auto getDefaultColor = [&defaultColor](const Ray & /*ray*/) {
        return defaultColor;
    };

    return  TraceRay(bodies, ray, getDefaultColor);
}

}

#endif // RTWE_TRACING_H
