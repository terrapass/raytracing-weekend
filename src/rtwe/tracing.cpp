#include "tracing.h"

#include "constants.h"
#include "math_utils.h"
#include "targets.h"

namespace rtwe
{

//
// Constants
//

constexpr int MAX_RAY_TRACE_DEPTH = 8;

//
// Service types
//

namespace
{

struct ScatteredRay
{
    Ray   Ray;
    Color Attenuation;
};

} // anonymous namespace

//
// Utilities
//

static inline std::optional<Color> TryRayTraceImpl(const std::vector<Body> & bodies, const Ray & ray, const int depth);

std::optional<Color> TryTraceRay(const std::vector<Body> & bodies, const Ray & ray)
{
    return TryRayTraceImpl(bodies, ray, 0);
}

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

static inline std::optional<float> TryRayHitPlaneImpl(const Ray & ray, const Vector3 & planePoint, const Vector3 & planeNormal);

std::optional<RayHit> TryRayHitPlane(const Ray & ray, const Vector3 & planePoint, const Vector3 & planeNormal)
{
    const std::optional<float> rayHitParam = TryRayHitPlaneImpl(ray, planePoint, planeNormal);

    if (!rayHitParam.has_value())
        return std::nullopt;

    RayHit rayHit;
    rayHit.RayParam  = rayHitParam.value();
    rayHit.Hitpoint  = ray.GetPointAtParameter(*rayHitParam);
    rayHit.RawNormal = planeNormal;

    return rayHit;
}

//
// Service
//

static inline Vector3 GetRandomPointInUnitSphere()
{
    while (true)
    {
        const Vector3 candidatePoint = Vector3::Random();
        if (candidatePoint.squaredNorm() < 1.0f)
            return candidatePoint;
    }
}

static inline std::optional<ScatteredRay> TryScatterLambertian(const Ray & /*ray*/, const RayHit & rayHit, const Material & material)
{
    const Vector3 scatterTarget = rayHit.Hitpoint + rayHit.RawNormal.normalized() + GetRandomPointInUnitSphere();

    return ScatteredRay{
        Ray(rayHit.Hitpoint, scatterTarget - rayHit.Hitpoint),
        material.Albedo
    };
}

static inline float GetRayHitSqrDistance(const Vector3 rayOrigin, const std::optional<RayHit> & rayHit)
{
    return rayHit.has_value()
        ? (rayHit->Hitpoint - rayOrigin).squaredNorm()
        : INFINITY;
}

static inline std::optional<Color> TryRayTraceImpl(const std::vector<Body> & bodies, const Ray & ray, const int depth)
{
    if (bodies.empty() || depth >= MAX_RAY_TRACE_DEPTH)
        return std::nullopt;

    // TODO: Optimize; add opportunity for early exit if there's no posibility to hit something closer (see CompositeRayTarget)

    std::vector<std::optional<RayHit>> rayHits(bodies.size());

    std::transform(
        bodies.cbegin(),
        bodies.cend(),
        rayHits.begin(),
        [&ray](const Body & body) {
            return body.RayTarget->TryHit(ray, 0.01f, INFINITY);
        }
    );

    const auto closestRayHitIt = std::min_element(
        rayHits.cbegin(),
        rayHits.cend(),
        [&ray](const std::optional<RayHit> & rayHit0, const std::optional<RayHit> & rayHit1) {
            return GetRayHitSqrDistance(ray.Origin, rayHit0) < GetRayHitSqrDistance(ray.Origin, rayHit1);
        }
    );
    assert(closestRayHitIt != rayHits.cend());

    if (!closestRayHitIt->has_value())
        return std::nullopt;

    const size_t   closestRayHitIndex = closestRayHitIt - rayHits.cbegin();
    const size_t   closestBodyIndex   = closestRayHitIndex;
    const RayHit & closestRayHit      = closestRayHitIt->value();
    const Body &   closestBody        = bodies[closestBodyIndex];

    const std::optional<ScatteredRay> scatteredRay = TryScatterLambertian(ray, closestRayHit, closestBody.Material);
    if (!scatteredRay.has_value())
        return Color::BLACK;

    const Color scatteredRayColor = TryRayTraceImpl(bodies, scatteredRay->Ray, depth + 1).value_or(MISSED_RAY_COLOR);

    return Color(
        scatteredRay->Attenuation.Rgb.x() * scatteredRayColor.Rgb.x(),
        scatteredRay->Attenuation.Rgb.y() * scatteredRayColor.Rgb.y(),
        scatteredRay->Attenuation.Rgb.z() * scatteredRayColor.Rgb.z()
    );
}

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

static inline std::optional<float> TryRayHitPlaneImpl(const Ray & ray, const Vector3 & planePoint, const Vector3 & planeNormal)
{
    const float denominator = ray.Direction.dot(planeNormal);

    // If the ray is parallel to the plane
    if (std::fabs(denominator) < EPSILON)
        return {};

    const float numerator = (planePoint - ray.Origin).dot(planeNormal);

    return (numerator/denominator);
}

}
