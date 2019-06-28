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
    m_Mutex               ()
{
    // Empty
}

} // namespace rtwe
