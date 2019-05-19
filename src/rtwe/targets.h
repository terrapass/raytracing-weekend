#ifndef RTWE_TARGETS_H
#define RTWE_TARGETS_H

#include <optional>
#include <memory>
#include <vector>

#include "types.h"
#include "Color.h"

namespace rtwe
{

//
// Forward declarations
//

struct Ray;
struct RayHit;

//
// IRayTarget
//

struct IRayTarget
{
    virtual ~IRayTarget() = default;

    virtual std::optional<RayHit> TryHit(
        const Ray & ray,
        const float minRayParam,
        const float maxRayParam
    ) const = 0;
};

//
// CompositeRayTarget
//

class CompositeRayTarget final:
    public IRayTarget
{
public: // Construction

    CompositeRayTarget(std::initializer_list<std::shared_ptr<IRayTarget>> targetsInitList);

public: // IRayTarget

    virtual std::optional<RayHit> TryHit(
        const Ray & ray,
        const float minRayParam,
        const float maxRayParam
    ) const override;

private: // Members

    std::vector<std::shared_ptr<IRayTarget>> m_Targets;
};

//
// SkyboxGradientRayTarget
//

class SkyboxGradientRayTarget final:
    public IRayTarget
{
public: // Construction

    SkyboxGradientRayTarget(Color bottomColor, Color topColor);

public: // IRayTarget

    virtual std::optional<RayHit> TryHit(
        const Ray & ray,
        const float minRayParam,
        const float maxRayParam
    ) const override;

private: // Members

    const Color m_BottomColor;
    const Color m_TopColor;
};

//
// PlaneRayTarget
//

class PlaneRayTarget final:
    public IRayTarget
{
public: // Construction

    PlaneRayTarget(Vector3 point, Vector3 normal);

public: // IRayTarget

    virtual std::optional<RayHit> TryHit(
        const Ray & ray,
        const float minRayParam,
        const float maxRayParam
    ) const override;

private:

    const Vector3 m_Point;
    const Vector3 m_Normal;
};

//
// SphereRayTarget
//

class SphereRayTarget final:
    public IRayTarget
{
public: // Construction

    SphereRayTarget(Vector3 center, const float radius);

public: // IRayTarget

    virtual std::optional<RayHit> TryHit(
        const Ray & ray,
        const float minRayParam,
        const float maxRayParam
    ) const override;

private: // Service

    bool CanHit(
        const Ray & ray,
        const float minRayParam,
        const float maxRayParam
    ) const;

private: // Members

    const Vector3 m_Center;
    const float   m_Radius;
};

}

#endif // RTWE_TARGETS_H
