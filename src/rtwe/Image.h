#ifndef RTWE_IMAGE_H
#define RTWE_IMAGE_H

#include <mutex>

#include "types.h"
#include "math_utils.h"
#include "Color.h"

namespace rtwe
{

class Image final
{
public: // Construction

    Image(const int width, const int height);

public: // Deleted

    Image(const Image &) = delete;

    Image & operator=(const Image &) = delete;

public: // Interface

    inline int GetWidth() const;

    inline int GetHeight() const;

    /**
     * @return true if further submissions might affect this pixel's value, false otherwise.
     */
    inline bool SubmitPixelRgb(const int x, const int y, const Vector3 & rgb);

    //inline void LockForReading() const; // TODO?

    inline Color GetPixelColor(const int x, const int y) const;

    //inline void UnlockAfterReading() const; // TODO?

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

private: // Service

    inline size_t ToPixelIndex(const int x, const int y) const;

    inline bool AreCoordinatesValid(const int x, const int y) const;

private: // Members

    const int m_Width;
    const int m_Height;

    std::vector<PixelRgbAccumulator> m_PixelRgbAccumulators;

    mutable std::mutex m_Mutex; // TODO: Use shared_mutex?
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

inline bool Image::SubmitPixelRgb(const int x, const int y, const Vector3 & rgb)
{
    std::unique_lock lock(m_Mutex);

    PixelRgbAccumulator & accumulator = m_PixelRgbAccumulators[ToPixelIndex(x, y)];

    accumulator.AccumulatedRgb += rgb;
    accumulator.Count++;

    const bool isPixelValueFinal = 1.0f/static_cast<float>(accumulator.Count) < EPSILON;

    return !isPixelValueFinal;
}

inline Color Image::GetPixelColor(const int x, const int y) const
{
    std::unique_lock lock(m_Mutex);

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
