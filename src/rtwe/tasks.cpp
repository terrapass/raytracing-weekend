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

RepeatingSampleImageBandTask::RepeatingSampleImageBandTask(
    SamplingThreadPool &      threadPool,
    Image &                   targetImage,
    const std::vector<Body> & scene,
    const Camera &            camera,
    const RayMissFunction     rayMissFunc,
    const int                 bandIndex
):
    m_pThreadPool (&threadPool),
    m_pTargetImage(&targetImage),
    m_pScene      (&scene),
    m_pCamera     (&camera),
    m_RayMissFunc (rayMissFunc),
    m_BandIndex   (bandIndex)
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

void RepeatingSampleImageBandTask::operator()()
{
    const std::pair<int, int> bandYRange = m_pTargetImage->GetBandYRange(m_BandIndex);

    bool isAnyPixelModifiable = false;
    for (int y = bandYRange.first; y < bandYRange.second; y++)
    {
        for (int x = 0; x < m_pTargetImage->GetWidth(); x++)
        {
            const Vector3 sampleColor = SamplePixelRgb(
                *m_pScene,
                *m_pCamera,
                m_RayMissFunc,
                m_pTargetImage->GetWidth(),
                m_pTargetImage->GetHeight(),
                x,
                y
            );

            // FIXME: This pattern of locks-unlocks is inefficient, since it happens on every iteration.
            //        However, taking it out of the loop won't help much because SamplePixelRgb() is a slow operation.
            //        Perhaps a different synchronization primitive than a mere std::mutex would be better suited here.
            m_pTargetImage->LockBand(m_BandIndex);

            if (m_pTargetImage->SubmitPixelRgb(x, y, sampleColor))
                isAnyPixelModifiable = true;

            m_pTargetImage->UnlockBand(m_BandIndex);
        }
    }

    // Repeat the task, if any pixels in the target band can still be modified
    if (isAnyPixelModifiable)
    {
        m_pThreadPool->EmplaceTask(std::move(*this));
    }
    else
    {
        BOOST_LOG_TRIVIAL(info)<<
            "All pixels in image band " << m_BandIndex <<
            " (y coords " << bandYRange.first << " to " << bandYRange.second <<
            " have reached their final values";
    }
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