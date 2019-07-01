#ifndef RTWE_IMAGE_H
#define RTWE_IMAGE_H

#include <shared_mutex>

#include "types.h"
#include "math_utils.h"
#include "Color.h"

namespace rtwe
{

class Image final
{
public: // Construction

    Image(const int width, const int height, const int bandsCountHint);

public: // Deleted

    Image(const Image &) = delete;

    Image & operator=(const Image &) = delete;

public: // Interface

    inline int GetWidth() const;

    inline int GetHeight() const;

    inline int GetBandsCount() const;

    inline std::pair<int, int> GetBandYRange(const int bandIndex) const;

    // TODO: See if the following methods may be replaced with one returning some RTTI entity.

    void LockBand(const int bandIndex) const;

    bool TryLockBand(const int bandIndex) const;

    /**
     * @return true if further submissions might affect this pixel's value, false otherwise.
     */
    inline bool SubmitPixelRgb(const int x, const int y, const Vector3 & rgb);

    inline Color GetPixelColor(const int x, const int y) const;

    void UnlockBand(const int bandIndex) const;

private: // Service types

    struct PixelRgbAccumulator final
    {
    public: // Attributes

        Vector3 AccumulatedRgb;
        long    Count;

    public: // Construction

        PixelRgbAccumulator():
            AccumulatedRgb(Vector3::Zero()),
            Count         (0)
        {
            // Empty
        }
    };

    struct Band final
    {
    public: // Attributes

        int MinY;
        int MaxY;

        mutable std::mutex Mutex;

#ifdef RTWE_DEBUG
        mutable bool IsMutexLocked;
#endif

    public: // Construction

        Band(const int minY, const int maxY):
            MinY (minY),
            MaxY (maxY),
            Mutex()
#ifdef RTWE_DEBUG
            , IsMutexLocked(false)
#endif
        {
            // Empty
        }

        Band(const Band & other):
            MinY (other.MinY),
            MaxY (other.MaxY),
            Mutex()
#ifdef RTWE_DEBUG
            , IsMutexLocked(false)
#endif
        {
            assert(!other.IsMutexLocked);
        }
    };

private: // Service

    inline size_t ToPixelIndex(const int x, const int y) const;

    inline bool AreCoordinatesValid(const int x, const int y) const;

    const Band & GetBandByPixelY(const int y) const;

    Band & GetBandByPixelY(const int y);

    std::vector<Band> CreateBands(const int bandsCountHint);

private: // Constants

    static constexpr int COLOR_COMPONENT_STEPS = 255;

    // After this many additions to PixelRgbAccumulator's AccumulatedRgb
    // resulting pixel value will not change further.
    static constexpr long MAX_ACCUMULATOR_COUNT =
        static_cast<long>(1.0f/(static_cast<float>(COLOR_COMPONENT_STEPS)*EPSILON));

private: // Members

    const int m_Width;
    const int m_Height;

    std::vector<PixelRgbAccumulator> m_PixelRgbAccumulators;

    std::vector<Band> m_Bands;
};

//
// Interface
//

inline int Image::GetWidth() const
{
    return m_Width;
}

inline int Image::GetHeight() const
{
    return m_Height;
}

inline int Image::GetBandsCount() const
{
    return static_cast<int>(m_Bands.size());
}

inline std::pair<int, int> Image::GetBandYRange(const int bandIndex) const
{
    const Band & band = m_Bands[bandIndex];
    return std::make_pair(band.MinY, band.MaxY);
}

inline bool Image::SubmitPixelRgb(const int x, const int y, const Vector3 & rgb)
{
    assert(GetBandByPixelY(y).IsMutexLocked && "SubmitPixelRgb() must be called after the corresponding image band has been locked with LockBand()");

    PixelRgbAccumulator & accumulator = m_PixelRgbAccumulators[ToPixelIndex(x, y)];

    accumulator.AccumulatedRgb += rgb;
    accumulator.Count++;

    const bool isPixelValueFinal = accumulator.Count >= MAX_ACCUMULATOR_COUNT;

    return !isPixelValueFinal;
}

inline Color Image::GetPixelColor(const int x, const int y) const
{
    assert(GetBandByPixelY(y).IsMutexLocked && "GetPixelColor() must be called after the corresponding image band has been locked with LockBand()");

    const PixelRgbAccumulator & accumulator = m_PixelRgbAccumulators[ToPixelIndex(x, y)];

    return Color(accumulator.AccumulatedRgb / static_cast<float>(accumulator.Count));
}

//
// Service
//

inline size_t Image::ToPixelIndex(const int x, const int y) const
{
    assert(AreCoordinatesValid(x, y));

    return y*m_Width + x;
}

inline bool Image::AreCoordinatesValid(const int x, const int y) const
{
    return x >= 0 && x < m_Width && y >= 0 && y < m_Height;
}

} // namespace rtwe

#endif // RTWE_IMAGE_H
