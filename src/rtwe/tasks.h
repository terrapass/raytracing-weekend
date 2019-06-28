#ifndef RTWE_TASKS_H
#define RTWE_TASKS_H

#include <vector>

#include "tracing.h"

namespace rtwe
{

//
// Forward declarations
//

template <typename Task>
class ThreadPool;

class Image;
struct Body;
class Camera;

//
//
//

struct RepeatingSamplePixelTask final
{
public: // Type aliases

    using SamplingThreadPool = ThreadPool<RepeatingSamplePixelTask>;

public: // Construction

    RepeatingSamplePixelTask(
        SamplingThreadPool &      threadPool,
        Image &                   targetImage,
        const std::vector<Body> & scene,
        const Camera &            camera,
        const RayMissFunction     rayMissFunc,
        const int                 pixelX,
        const int                 pixelY
    );

    RepeatingSamplePixelTask(RepeatingSamplePixelTask &&) = default;

public: // Interface

    void operator()();

private: // Members

    SamplingThreadPool *      m_pThreadPool;
    Image *                   m_pTargetImage;
    const std::vector<Body> * m_pScene;
    const Camera *            m_pCamera;

    RayMissFunction m_RayMissFunc;
    int             m_PixelX;
    int             m_PixelY;
};

}

#endif // RTWE_TASKS_H
