#include "Application.h"

#include <cassert>
#include <boost/log/trivial.hpp>

#include <sdl2utils/event_utils.h>
#include <sdl2utils/guards.h>

#include "constants.h"
#include "tracing.h"
#include "targets.h"
#include "Camera.h"

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

static inline float GetRandomValue();
static inline Color RawNormalToColor(const Vector3 & rawNormal);

int Application::run()
{
    const sdl2utils::SDL_WindowPtr window = createWindow();
    assert(window);

    const sdl2utils::SDL_RendererPtr renderer = createRenderer(window.get());
    assert(renderer);

    const sdl2utils::SDL_TexturePtr streamingTexture = createStreamingTexture(renderer.get());
    assert(streamingTexture);

    const std::unique_ptr<IRayTarget> raytracingScene = createRaytracingScene();
    assert(raytracingScene);

    void * pixels = nullptr;
    int    pitch  = -1;

    const int lockResult = SDL_LockTexture(streamingTexture.get(), nullptr, &pixels, &pitch);
    assert(lockResult == 0 && "SDL_LockTexture() must succeed");

    static const float PROJECTION_HEIGHT = 2.0f;
    static const float PROJECTION_WIDTH  = PROJECTION_HEIGHT * WINDOW_ASPECT_RATIO;

    static const int SAMPLES_PER_PIXEL = IS_RELEASE
        ? 128
        : 1;

    // The following code uses a left-handed coordinate system:
    // x points right, y points up, z points into the screen.

    static const Vector3 CAMERA_ORIGIN    (0.0f, 0.0f, 0.0f);
    static const Vector3 PROJECTION_CENTER(0.0f, 0.0f, 1.0f);

    const Camera camera(
        CAMERA_ORIGIN,
        PROJECTION_CENTER,
        PROJECTION_WIDTH,
        PROJECTION_HEIGHT
    );

    for (int y = 0; y < WINDOW_HEIGHT; y++)
    {
        for (int x = 0; x < WINDOW_WIDTH; x++)
        {
            const int      pixelOffset = y*pitch + x*sizeof(Uint32);
            Uint32 * const pixel       = reinterpret_cast<Uint32 *>(reinterpret_cast<Uint8 *>(pixels) + pixelOffset);

            Vector3 accumulatedRawNormal = Vector3::Zero();

            for (int sampleIdx = 0; sampleIdx < SAMPLES_PER_PIXEL; sampleIdx++)
            {
                const float sampleX = (static_cast<float>(x) + GetRandomValue() - 0.5f);
                const float sampleY = (static_cast<float>(y) + GetRandomValue() - 0.5f);

                const float normalizedSampleX = sampleX/static_cast<float>(WINDOW_WIDTH);
                const float normalizedSampleY = 1.0f - sampleY/static_cast<float>(WINDOW_HEIGHT);

                const Ray ray = camera.CreateRay(normalizedSampleX, normalizedSampleY);

                const std::optional<RayHit> rayHit = raytracingScene->TryHit(ray, 0.0f, INFINITY);
                assert(rayHit.has_value());

                accumulatedRawNormal += rayHit->RawNormal;
            }

            const Vector3 averageRawNormal = accumulatedRawNormal/static_cast<float>(SAMPLES_PER_PIXEL);

            *pixel = RawNormalToColor(averageRawNormal).ToArgb();
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

static inline float GetRandomValue()
{
    return drand48(); // TODO: Replace with a call to C++ API
}

static inline Color RawNormalToColor(const Vector3 & rawNormal)
{
    const Vector3 normal            = rawNormal.normalized();
    const Vector3 nonNegativeNormal = 0.5f*(normal + Vector3(1.0f, 1.0f, 1.0f));

    return Color(nonNegativeNormal);
}

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

std::unique_ptr<IRayTarget> Application::createRaytracingScene()
{
    return std::make_unique<CompositeRayTarget>(
        std::initializer_list<std::shared_ptr<IRayTarget>>{
            std::make_shared<SkyboxGradientRayTarget>(Color::WHITE, Color::BLUE),
            std::make_shared<PlaneRayTarget>(Vector3(0.0f, -0.5f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
            std::make_shared<SphereRayTarget>(Vector3(0.0f, 0.0f, 1.0f), 0.5f)
        }
    );
}

} // namespace rtwe