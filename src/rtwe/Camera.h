#ifndef RTWE_CAMERA_H
#define RTWE_CAMERA_H

#include "types.h"
#include "Ray.h"

namespace rtwe
{

class Camera final
{
public: // Construction

    inline Camera(
        Vector3       origin,
        Vector3       projectionCenter,
        const Vector3 up,
        const float   projectionWidth,
        const float   projectionHeight
    );

public: // Interface

    inline Ray CreateRay(const float normalizedTargetX, const float normalizedTargetY) const;

private: // Members

    const Vector3 m_Origin;
    const Vector3 m_ProjectionCenter;
    const Vector3 m_ProjectionUp;
    const Vector3 m_ProjectionRight;
};

//
// Construction
//

inline Camera::Camera(
    Vector3       origin,
    Vector3       projectionCenter,
    const Vector3 up,
    const float   projectionWidth,
    const float   projectionHeight
):
    m_Origin          (std::move(origin)),
    m_ProjectionCenter(std::move(projectionCenter)),
    m_ProjectionUp    (projectionHeight * up.normalized()),
    m_ProjectionRight (projectionWidth  * up.cross(m_ProjectionCenter - m_Origin).normalized())
{
    // Empty
}

//
// Interface
//

inline Ray Camera::CreateRay(const float normalizedTargetX, const float normalizedTargetY) const
{
    const Vector3 raytracingTarget =
        m_ProjectionCenter + 
        m_ProjectionRight * (normalizedTargetX - 0.5f) +
        m_ProjectionUp    * (normalizedTargetY - 0.5f);

    return Ray(
        m_Origin,
        raytracingTarget - m_Origin
    );
}

} // namespace rtwe

#endif // RTWE_CAMERA_H
