#include "targets.h"

#include "tracing.h"

namespace rtwe
{

//
// CompositeRayTarget
//

//
// Construction
//

CompositeRayTarget::CompositeRayTarget(std::initializer_list<std::shared_ptr<IRayTarget>> targetsInitList):
    m_Targets(targetsInitList)
{
    // Empty
}

//
// IRayTarget
//

std::optional<RayHit> CompositeRayTarget::TryHit(
    const Ray & ray,
    const float minRayParam,
    const float maxRayParam
) const
{
    std::optional<RayHit> result;
    float currentMaxRayParam = maxRayParam;
    for (const std::shared_ptr<IRayTarget> & target : m_Targets)
    {
        if (currentMaxRayParam < minRayParam)
            break;

        if (std::optional<RayHit> targetHit = target->TryHit(ray, minRayParam, currentMaxRayParam))
        {
            assert(targetHit->RayParam < currentMaxRayParam);

            currentMaxRayParam = targetHit->RayParam;
            result             = std::move(targetHit);
        }
    }

    return result;
}

//
//
//

//
// SkyboxGradientRayTarget
//

// TODO

//
//
//

//
// SphereRayTarget
//

SphereRayTarget::SphereRayTarget(Vector3 center, const float radius):
    m_Center(std::move(center)),
    m_Radius(radius)
{
    // Empty
}

std::optional<RayHit> SphereRayTarget::TryHit(
    const Ray & ray,
    const float minRayParam,
    const float maxRayParam
) const
{
    std::optional<RayHit> rayHit = TryRayHitSphere(ray, m_Center, m_Radius);

    if (!rayHit.has_value() || rayHit->RayParam < minRayParam || rayHit->RayParam > maxRayParam)
        return std::nullopt;

    return rayHit;
}

//
//
//

//
// PlaneRayTarget
//

// TODO

//
//
//

}
