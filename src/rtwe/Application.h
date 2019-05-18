#ifndef RTWE_APPLICATION_H
#define RTWE_APPLICATION_H

#include <sdl2utils/raii.h>
#include <sdl2utils/pointers.h>

//
//
//

namespace rtwe
{

class Application final
{
public: // Construction

    explicit Application();

public: // Deleted

    Application(const Application&) = delete;
    Application(Application&&)      = delete;

    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&)      = delete;

public: // Interface

    int run();

private: // Service

    static sdl2utils::SDL_WindowPtr createWindow();

    static sdl2utils::SDL_RendererPtr createRenderer(SDL_Window * const pWindow);

    static sdl2utils::SDL_TexturePtr createStreamingTexture(SDL_Renderer * const pRenderer);

private: // Constants

    static const int WINDOW_WIDTH;
    static const int WINDOW_HEIGHT;

    static const float WINDOW_ASPECT_RATIO;

    static const char * const WINDOW_TITLE;

    static const int    SDL_INIT_FLAGS;
    static const Uint32 SDL_TEXTURE_PIXELFORMAT;

private: // Members

    const sdl2utils::raii::ScopedSDLCore m_ScopedSDLCore;
};

} // namespace rtwe

#endif // RTWE_APPLICATION_H
