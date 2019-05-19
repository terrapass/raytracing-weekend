#include "Application.h"

#include <cassert>
#include <boost/log/trivial.hpp>

#include <sdl2utils/event_utils.h>
#include <sdl2utils/guards.h>

#include "tracing.h"

namespace rtwe
{

//
// Constants
//

const int Application::WINDOW_WIDTH  = 800;
const int Application::WINDOW_HEIGHT = 600;

const float Application::WINDOW_ASPECT_RATIO =
    static_cast<float>(Application::WINDOW_WIDTH)/static_cast<float>(Application::WINDOW_HEIGHT);

const char * const Application::WINDOW_TITLE = "Ray Tracing Weekend";

const int    Application::SDL_INIT_FLAGS          = SDL_INIT_EVERYTHING;
const Uint32 Application::SDL_TEXTURE_PIXELFORMAT = SDL_PIXELFORMAT_ARGB8888;

//
// Construction
//

Application::Application():
    m_ScopedSDLCore(SDL_INIT_FLAGS)
{
    // Empty
}

//
// Interface
//

int Application::run()
{
    const sdl2utils::SDL_WindowPtr window = createWindow();
    assert(window);

    const sdl2utils::SDL_RendererPtr renderer = createRenderer(window.get());
    assert(renderer);

    const sdl2utils::SDL_TexturePtr streamingTexture = createStreamingTexture(renderer.get());
    assert(streamingTexture);

    void * pixels = nullptr;
    int    pitch  = -1;

    const int lockResult = SDL_LockTexture(streamingTexture.get(), nullptr, &pixels, &pitch);
    assert(lockResult == 0 && "SDL_LockTexture() must succeed");

    static const float PROJECTION_HEIGHT = 2.0f;
    static const float PROJECTION_WIDTH  = PROJECTION_HEIGHT * WINDOW_ASPECT_RATIO;

    // The following code uses a left-handed coordinate system:
    // x points right, y points up, z points into the screen.

    const Vector3 raytracingOrigin      (0.0f, 0.0f, 0.0f);
    const Vector3 raytracingScreenCenter(0.0f, 0.0f, 1.0f);

    for (int y = 0; y < WINDOW_HEIGHT; y++)
    {
        for (int x = 0; x < WINDOW_WIDTH; x++)
        {
            const int      pixelOffset = y*pitch + x*sizeof(Uint32);
            Uint32 * const pixel       = reinterpret_cast<Uint32 *>(reinterpret_cast<Uint8 *>(pixels) + pixelOffset);

            const float normalizedPixelX = static_cast<float>(x)/static_cast<float>(WINDOW_WIDTH);
            const float normalizedPixelY = static_cast<float>(y)/static_cast<float>(WINDOW_HEIGHT);

            Vector3 raytracingTarget = raytracingScreenCenter;
            raytracingTarget.x() += PROJECTION_WIDTH  * (normalizedPixelX - 0.5f);
            raytracingTarget.y() += PROJECTION_HEIGHT * (normalizedPixelY - 0.5f);

            const Ray ray(
                raytracingOrigin,
                raytracingTarget - raytracingOrigin
            );

            *pixel = GetMissedRayColor(ray).ToArgb();
        }
    }

    SDL_UnlockTexture(streamingTexture.get());

    SDL_RenderCopy(renderer.get(), streamingTexture.get(), nullptr, nullptr);
    SDL_RenderPresent(renderer.get());

    sdl2utils::waitEscOrCrossPressed();

    return 0;
}

//
// Service
//

sdl2utils::SDL_WindowPtr Application::createWindow()
{
    return sdl2utils::SDL_WindowPtr(
        sdl2utils::guards::ensureNotNull(
            SDL_CreateWindow(
                WINDOW_TITLE,
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                WINDOW_WIDTH,
                WINDOW_HEIGHT,
                0u
            ),
            "result of SDL_CreateWindow()"
        )
    );
}

static bool doesRendererSupportPixelFormat(SDL_Renderer * pRenderer, const Uint32 pixelFormat)
{
    assert(pRenderer != nullptr);

    SDL_RendererInfo rendererInfo;
    const int result = SDL_GetRendererInfo(pRenderer, &rendererInfo);
    assert(result == 0 && "SDL_GetRendererInfo() must succeed");

    for (Uint32 i = 0; i < rendererInfo.num_texture_formats; i++)
    {
        if (rendererInfo.texture_formats[i] == pixelFormat)
            return true;
    }

    return false;
}

sdl2utils::SDL_RendererPtr Application::createRenderer(SDL_Window * const pWindow)
{
    sdl2utils::SDL_RendererPtr renderer(
        sdl2utils::guards::ensureNotNull(
            SDL_CreateRenderer(pWindow, -1, 0),
            "result of SDL_CreateRenderer()"
        )
    );

    if (!doesRendererSupportPixelFormat(renderer.get(), SDL_TEXTURE_PIXELFORMAT))
        BOOST_LOG_TRIVIAL(error) << "The renderer does not directly support texture pixel format ARGB8888; rendering will be slow due to conversions";

    return renderer;
}

sdl2utils::SDL_TexturePtr Application::createStreamingTexture(SDL_Renderer * const pRenderer)
{
    return sdl2utils::SDL_TexturePtr(
        sdl2utils::guards::ensureNotNull(
            SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT),
            "result of SDL_CreateTexture()"
        )
    );
}

} // namespace rtwe