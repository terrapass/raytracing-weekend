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
        Vector3 origin,
        Vector3 projectionCenter,
        const float projectionWidth,
        const float projectionHeight
    );

public: // Interface

    inline Ray CreateRay(const float normalizedTargetX, const float normalizedTargetY) const;

private: // Members

    const Vector3 m_Origin;
    const Vector3 m_ProjectionCenter;
    const float   m_ProjectionWidth;
    const float   m_ProjectionHeight;
};

//
// Construction
//

inline Camera::Camera(
    Vector3 origin,
    Vector3 projectionCenter,
    const float projectionWidth,
    const float projectionHeight
):
    m_Origin          (std::move(origin)),
    m_ProjectionCenter(std::move(projectionCenter)),
    m_ProjectionWidth (projectionWidth),
    m_ProjectionHeight(projectionHeight)
{
    // Empty
}

//
// Interface
//

inline Ray Camera::CreateRay(const float normalizedTargetX, const float normalizedTargetY) const
{
    Vector3 raytracingTarget = m_ProjectionCenter;
    raytracingTarget.x() += m_ProjectionWidth  * (normalizedTargetX - 0.5f);
    raytracingTarget.y() += m_ProjectionHeight * (normalizedTargetY - 0.5f);

    return Ray(
        m_Origin,
        raytracingTarget - m_Origin
    );
}

}

#endif // RTWE_CAMERA_H
