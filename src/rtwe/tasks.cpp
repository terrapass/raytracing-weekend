#include "tasks.h"

#include "threading.h"
#include "math_utils.h"
#include "Image.h"
#include "Camera.h"

namespace rtwe
{

//
// Construction
//

RepeatingSamplePixelTask::RepeatingSamplePixelTask(
    SamplingThreadPool &      threadPool,
    Image &                   targetImage,
    const std::vector<Body> & scene,
    const Camera &            camera,
    const RayMissFunction     rayMissFunc,
    const int                 pixelX,
    const int                 pixelY
):
    m_pThreadPool (&threadPool),
    m_pTargetImage(&targetImage),
    m_pScene      (&scene),
    m_pCamera     (&camera),
    m_RayMissFunc (rayMissFunc),
    m_PixelX      (pixelX),
    m_PixelY      (pixelY)
{
    // Empty
}

//
// Interface
//

static inline Vector3 SamplePixelRgb(
    const std::vector<Body> & scene,
    const Camera &            camera,
    const RayMissFunction     rayMissFunc,
    const int                 imageWidth,
    const int                 imageHeight,
    const int                 pixelX,
    const int                 pixelY
);

void RepeatingSamplePixelTask::operator()()
{
    const Vector3 sampleColor = SamplePixelRgb(
        *m_pScene,
        *m_pCamera,
        m_RayMissFunc,
        m_pTargetImage->GetWidth(),
        m_pTargetImage->GetHeight(),
        m_PixelX,
        m_PixelY
    );

    // Repeat the task, if the target pixel can still be modified
    if (m_pTargetImage->SubmitPixelRgb(m_PixelX, m_PixelY, sampleColor))
        m_pThreadPool->EmplaceTask(std::move(*this));
}

//
// Service
//

static inline Vector3 SamplePixelRgb(
    const std::vector<Body> & scene,
    const Camera &            camera,
    const RayMissFunction     rayMissFunc,
    const int                 imageWidth,
    const int                 imageHeight,
    const int                 pixelX,
    const int                 pixelY
)
{
    const float sampleX = (static_cast<float>(pixelX) + GetRandomValue() - 0.5f);
    const float sampleY = (static_cast<float>(pixelY) + GetRandomValue() - 0.5f);

    const float normalizedSampleX = sampleX/static_cast<float>(imageWidth);
    const float normalizedSampleY = 1.0f - sampleY/static_cast<float>(imageHeight);

    const Ray ray = camera.CreateRay(normalizedSampleX, normalizedSampleY);

    const Color rayColor = TraceRay(
        scene,
        ray,
        rayMissFunc
    );

    return rayColor.Rgb;
}

} // namespace rtwe
