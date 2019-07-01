#include "Image.h"

#include <algorithm>

namespace rtwe
{

//
// Construction
//

Image::Image(const int width, const int height, const int bandsCountHint):
    m_Width               (width),
    m_Height              (height),
    m_PixelRgbAccumulators(m_Width * m_Height),
    m_Bands               (CreateBands(bandsCountHint))
{
    // Empty
}

//
// Interface
//

void Image::LockBand(const int bandIndex) const
{
    const Band & band = m_Bands[bandIndex];

    band.Mutex.lock();

#ifdef RTWE_DEBUG
    band.IsMutexLocked = true;
#endif
}

bool Image::TryLockBand(const int bandIndex) const
{
    const Band & band = m_Bands[bandIndex];

    const bool result = band.Mutex.try_lock();

#ifdef RTWE_DEBUG
    band.IsMutexLocked = band.IsMutexLocked || result;
#endif

    return result;
}

void Image::UnlockBand(const int bandIndex) const
{
    const Band & band = m_Bands[bandIndex];

    assert(band.IsMutexLocked && "UnlockBand() must not be called unless the band is locked");

#ifdef RTWE_DEBUG
    band.IsMutexLocked = false;
#endif

    band.Mutex.unlock();
}

//
// Service
//

const Image::Band & Image::GetBandByPixelY(const int y) const
{
    assert(y >= 0);
    assert(y < GetHeight());

    const auto bandIt = std::lower_bound(
        m_Bands.cbegin(),
        m_Bands.cend(),
        y,
        [](const Band & band, const int yValue)
        {
            return band.MaxY < yValue;
        }
    );

    assert(bandIt != m_Bands.cend());

    return (*bandIt);
}

Image::Band & Image::GetBandByPixelY(const int y)
{
    // Scott Meyers said this is OK: https://stackoverflow.com/a/123995
    return const_cast<Image::Band &>(const_cast<const Image &>(*this).GetBandByPixelY(y));
}

std::vector<Image::Band> Image::CreateBands(const int bandsCountHint)
{
    const int bandsCount = std::min(bandsCountHint, m_Height);

    const int regularBandHeight = m_Height / bandsCount;
    const int lastBandHeight    = regularBandHeight + (m_Height % bandsCount);
    assert(lastBandHeight > 0);

    assert((bandsCount - 1)*regularBandHeight + lastBandHeight == m_Height);

    std::vector<Band> bands;
    bands.reserve(bandsCount);

    int lastMaxY = 0;
    for (int i = 0; i < bandsCountHint - 1; i++)
    {
        const int minY = lastMaxY;

        lastMaxY += regularBandHeight;

        bands.emplace_back(
            minY,
            lastMaxY
        );
    }

    assert(lastMaxY + lastBandHeight == m_Height);

    bands.emplace_back(lastMaxY, m_Height);

    return bands;
}

} // namespace rtwe
