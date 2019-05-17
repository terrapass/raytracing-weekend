#ifndef RTWE_RAY_H
#define RTWE_RAY_H

#include "types.h"

namespace rtwe
{

struct Ray final
{
public: // Attributes

    Vector3 Origin;
    Vector3 Direction;

public: // Construction

    Ray() = default;

    inline Ray(Vector3 origin, Vector3 direction);

public: // Interface

    inline Vector3 GetPointAtParameter(const float param) const;
};

//
// Construction
//

inline Ray::Ray(Vector3 origin, Vector3 direction):
    Origin   (std::move(origin)),
    Direction(std::move(direction))
{
    // Empty
}

//
// Interface
//

inline Vector3 Ray::GetPointAtParameter(const float param) const
{
    return Origin + param*Direction;
}

} // namespace rtwe

#endif // RTWE_RAY_H