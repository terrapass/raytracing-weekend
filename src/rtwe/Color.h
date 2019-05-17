#ifndef RTWE_COLOR_H
#define RTWE_COLOR_H

#include "types.h"

namespace rtwe
{

struct Color final
{
public: // Constants

    static const Color BLACK;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color MAGENTA;
    static const Color CYAN;
    static const Color WHITE;

public: // Attributes

    Vector3 Rgb;

public: // Construction

    Color() = default;

    inline explicit Color(Vector3 rgb);

    inline Color(const float red, const float green, const float blue);

public: // Interface

    inline Uint32 ToArgb() const;

    inline Uint32 ToArgbWithAlpha(const Uint8 alpha) const;

    inline Uint32 ToArgbWithAlpha(const float alpha) const;

private: // Service

    static inline Uint8 ToColorComponent(const float value);
};

//
// Construction
//

inline Color::Color(Vector3 rgb):
    Rgb(std::move(rgb))
{
    // Empty
}

inline Color::Color(const float red, const float green, const float blue):
    Rgb(red, green, blue)
{
    // Empty
}

//
// Interface
//

inline Uint32 Color::ToArgb() const
{
    return ToArgbWithAlpha(static_cast<Uint8>(0xFF));
}

inline Uint32 Color::ToArgbWithAlpha(const Uint8 alpha) const
{
    const Uint8 red   = ToColorComponent(Rgb[0]);
    const Uint8 green = ToColorComponent(Rgb[1]);
    const Uint8 blue  = ToColorComponent(Rgb[2]);

    return (alpha << 24) |
           (red   << 16) |
           (green << 8)  |
           (blue  << 0);
}

inline Uint32 Color::ToArgbWithAlpha(const float alpha) const
{
    return ToArgbWithAlpha(ToColorComponent(alpha));
}

//
// Service
//

inline Uint8 Color::ToColorComponent(const float value)
{
    static const float MAX_COMPONENT_VALUE = static_cast<float>(0xFF);

    return static_cast<Uint8>(MAX_COMPONENT_VALUE * value);
}

}

#endif
