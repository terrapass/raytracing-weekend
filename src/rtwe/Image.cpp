#include "Image.h"

namespace rtwe
{

//
// Construction
//

Image::Image(const int width, const int height):
    m_Width               (width),
    m_Height              (height),
    m_PixelRgbAccumulators(m_Width * m_Height),
    m_ReadingMutex        ()
#ifdef RTWE_DEBUG
    , m_IsReadingMutexLockedExclusively(false)
#endif
{
    // Empty
}

//
// Interface
//

void Image::LockForReading() const
{
    assert(!m_IsReadingMutexLockedExclusively && "LockForReading() must not be called repeatedly without UnlockAfterReading()");

    m_ReadingMutex.lock();

#ifdef RTWE_DEBUG
    m_IsReadingMutexLockedExclusively = true;
#endif
}

void Image::UnlockAfterReading() const
{
    assert(m_IsReadingMutexLockedExclusively && "UnlockAfterReading() must not be called unless the image is locked for reading");

    m_ReadingMutex.unlock();

#ifdef RTWE_DEBUG
    m_IsReadingMutexLockedExclusively = false;
#endif
}

} // namespace rtwe
