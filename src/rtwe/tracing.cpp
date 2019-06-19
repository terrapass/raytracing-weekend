#include "tracing.h"

#include <cassert>
#include <boost/log/trivial.hpp>

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

/**
 * @brief Calculates reflection probability for dielectrics using Schlick's approximation.
 * 
 * @param cosTheta Cosine of the angle between incident ray and surface normal.
 * @param environmentRefractiveIndex Refractive index (eta) of the medium from which the ray exits.
 * @param bodyRefractiveIndex Refractive index (eta) of the medium which the ray enters.
 * 
 * @return Reflection probability (between 0.0f and 1.0f).
 */
static inline float GetSchlickReflectivity(
    const float cosTheta,
    const float environmentRefractiveIndex,
    const float bodyRefractiveIndex
)
{
    assert(cosTheta >= -1.0f && cosTheta <= 1.0f);

    const float sqrtMinReflectivity =
        (environmentRefractiveIndex - bodyRefractiveIndex)/(environmentRefractiveIndex + bodyRefractiveIndex);

    const float minReflectivity = sqrtMinReflectivity*sqrtMinReflectivity;

    return minReflectivity + (1.0f - minReflectivity)*std::pow(1.0f - cosTheta, 5.0f);
}

static inline std::optional<ScatteredRay> TryScatterRefractive(
    const Ray &      ray,
    const RayHit &   rayHit,
    const Material & material
)
{
    const Vector3 incident      = ray.Direction.normalized(); // TODO: See if this is the same before and after determining doesRayExitNoraml
    const Vector3 surfaceNormal = rayHit.RawNormal.normalized(); // TODO: don't confuse with outward normal
    const float   dotProduct    = incident.dot(surfaceNormal);

    const bool doesRayExitBody = (dotProduct > 0.0f);

    const float refractiveRatio = doesRayExitBody
        ? material.RefractiveIndex/ENVIRONMENT_REFRACTIVE_INDEX
        : ENVIRONMENT_REFRACTIVE_INDEX/material.RefractiveIndex;

    const Vector3 outwardNormal = doesRayExitBody
        ? -surfaceNormal
        : surfaceNormal;

    const float discriminant = 1.0f - refractiveRatio*refractiveRatio * (1.0f - dotProduct*dotProduct);

    const bool canRefract = (discriminant >= 0.0f);
    if (!canRefract)
    {
        // If unable to refract, reflect the ray
        return TryScatterMetallic(ray, rayHit, material);
    }

    const float reflectionProbability = GetSchlickReflectivity(
        doesRayExitBody ? refractiveRatio*incident.dot(surfaceNormal) : -incident.dot(surfaceNormal),
        doesRayExitBody ? material.RefractiveIndex                    : ENVIRONMENT_REFRACTIVE_INDEX,
        doesRayExitBody ? ENVIRONMENT_REFRACTIVE_INDEX                : material.RefractiveIndex
    );

    if (GetRandomValue() < reflectionProbability)
    {
        // Reflect the ray
        return TryScatterMetallic(ray, rayHit, material);
    }

    const Vector3 refractDirection = refractiveRatio * (incident - outwardNormal*dotProduct) - outwardNormal*std::sqrt(discriminant);

    return ScatteredRay{
        Ray(rayHit.Hitpoint, refractDirection),
        Color::WHITE
    };
}

static inline float GetRayHitSqrDistance(const Vector3 rayOrigin, const std::optional<RayHit> & rayHit)
{
    return rayHit.has_value()
        ? (rayHit->Hitpoint - rayOrigin).squaredNorm()
        : INFINITY;
}

using ScatterFunc = std::optional<ScatteredRay> (*) (
    const Ray &      ray,
    const RayHit &   rayHit,
    const Material & material
);

static inline Color GetScatteredRayColor(
    const ScatterFunc         scatterFunc,
    const std::vector<Body> & bodies,
    const Ray &               ray,
    const RayMissFunction &   rayMissFunction,
    const int                 depth,
    const RayHit &            rayHit,
    const Material &          material
)
{
    const std::optional<ScatteredRay> scatteredRay = scatterFunc(
        ray,
        rayHit,
        material
    );

    if (scatteredRay.has_value())
    {
        const Color scatteredRayColor = TraceRayImpl(
            bodies,
            scatteredRay->Ray,
            rayMissFunction,
            depth + 1
        );

        return Color(multiplyElements(
            scatteredRay->Attenuation.Rgb,
            scatteredRayColor.Rgb
        ));
    }
    else
    {
        return Color::BLACK;
    }
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

    // Select scattering function via roulette-wheel, using Reflectivity, (1.0 - Reflectivity), and Transparency as weights.

    const float scatterFuncsWeightSum = 1.0f + closestBodyMaterial.Transparency;
    ScatterFunc selectedScatterFunc   = nullptr;

    float randomValue = GetRandomValue()*scatterFuncsWeightSum;

    if ((randomValue -= closestBodyMaterial.Reflectivity) < 0.0f)
        selectedScatterFunc = TryScatterMetallic;
    else if ((randomValue -= closestBodyMaterial.Transparency) < 0.0f)
        selectedScatterFunc = TryScatterRefractive;
    else
        selectedScatterFunc = TryScatterLambertian;

    return GetScatteredRayColor(
        selectedScatterFunc,
        bodies,
        ray,
        rayMissFunction,
        depth,
        closestRayHit,
        closestBodyMaterial
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
