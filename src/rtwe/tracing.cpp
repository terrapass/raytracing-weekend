#include "tracing.h"

#include <cassert>

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

static inline Color TraceRayImpl(
    const std::vector<Body> & bodies,
    const Ray &               ray,
    const RayMissFunction &   rayMissFunction,
    const int                 depth
);

Color TraceRay(const std::vector<Body> & bodies, const Ray & ray, const RayMissFunction & rayMissFunction)
{
    return TraceRayImpl(bodies, ray, rayMissFunction, 0);
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

static inline std::optional<ScatteredRay> TryScatterLambertian(
    const Ray &      /*ray*/,
    const RayHit &   rayHit,
    const Material & material
)
{
    const Vector3 scatterTarget = rayHit.Hitpoint + rayHit.RawNormal.normalized() + GetRandomPointInUnitSphere();

    return ScatteredRay{
        Ray(rayHit.Hitpoint, scatterTarget - rayHit.Hitpoint),
        material.Albedo
    };
}

static inline std::optional<ScatteredRay> TryScatterMetallic(
    const Ray &      ray,
    const RayHit &   rayHit,
    const Material & material
)
{
    const Vector3 incident = ray.Direction.normalized();
    const Vector3 normal   = rayHit.RawNormal.normalized();

    const Vector3 rawScatterDirection = incident - 2 * incident.dot(normal) * normal;

    assert(material.Smoothness >= 0.0f && material.Smoothness <= 1.0f);
    if (isAlmostEqual(material.Smoothness, 1.0f))
    {
        return ScatteredRay{
            Ray(rayHit.Hitpoint, rawScatterDirection),
            material.Albedo
        };
    }

    const float   fuzziness        = 1.0f - material.Smoothness;
    const Vector3 fuzzOffset       = fuzziness*GetRandomPointInUnitSphere();
    const Vector3 scatterDirection = rawScatterDirection + fuzzOffset;

    const bool isScatterBelowSurface = scatterDirection.dot(normal) <= 0.0f;
    if (isScatterBelowSurface)
        return std::nullopt;

    return ScatteredRay{
        Ray(rayHit.Hitpoint, scatterDirection.normalized()),
        material.Albedo
    };
}

static inline float GetRayHitSqrDistance(const Vector3 rayOrigin, const std::optional<RayHit> & rayHit)
{
    return rayHit.has_value()
        ? (rayHit->Hitpoint - rayOrigin).squaredNorm()
        : INFINITY;
}

static inline Color TraceRayImpl(
    const std::vector<Body> & bodies,
    const Ray &               ray,
    const RayMissFunction &   rayMissFunction,
    const int                 depth
)
{
    if (bodies.empty() || depth >= MAX_RAY_TRACE_DEPTH)
        return rayMissFunction(ray);

    // TODO: Optimize; add opportunity for early exit if there's no posibility to hit something closer (see CompositeRayTarget)

    std::vector<std::optional<RayHit>> rayHits(bodies.size());

    std::transform(
        bodies.cbegin(),
        bodies.cend(),
        rayHits.begin(),
        [&ray](const Body & body) {
            return body.RayTarget->TryHit(ray, RAYTRACE_MIN_RAY_PARAM, INFINITY);
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
        return rayMissFunction(ray);

    const size_t     closestRayHitIndex  = closestRayHitIt - rayHits.cbegin();
    const size_t     closestBodyIndex    = closestRayHitIndex;
    const RayHit &   closestRayHit       = closestRayHitIt->value();
    const Body &     closestBody         = bodies[closestBodyIndex];
    const Material & closestBodyMaterial = closestBody.Material;

    Vector3 attenuatedScatteredRgb = Vector3::Zero();

    // Lambertian component (if not fully metallic)
    if (!isAlmostEqual(closestBodyMaterial.Reflectivity, 1.0f))
    {
        const std::optional<ScatteredRay> lambertScatteredRay = TryScatterLambertian(
            ray,
            closestRayHit,
            closestBodyMaterial
        );

        if (lambertScatteredRay.has_value())
        {
            const Color lambertScatteredRayColor = TraceRayImpl(
                bodies,
                lambertScatteredRay->Ray,
                rayMissFunction,
                depth + 1
            );

            const float lambertContribution = (1.0f - closestBodyMaterial.Reflectivity);

            attenuatedScatteredRgb += lambertContribution * multiplyElements(
                lambertScatteredRay->Attenuation.Rgb,
                lambertScatteredRayColor.Rgb
            );
        }
    }

    // Metallic component (if not fully diffuse)
    {
        const std::optional<ScatteredRay> metalScatteredRay = TryScatterMetallic(
            ray,
            closestRayHit,
            closestBodyMaterial
        );

        if (metalScatteredRay.has_value())
        {
            const Color metalScatteredRayColor = TraceRayImpl(
                bodies,
                metalScatteredRay->Ray,
                rayMissFunction,
                depth + 1
            );

            const float metalContribution = closestBodyMaterial.Reflectivity;

            attenuatedScatteredRgb += metalContribution * multiplyElements(
                metalScatteredRay->Attenuation.Rgb,
                metalScatteredRayColor.Rgb
            );
        }
    }

    return Color(attenuatedScatteredRgb);
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
