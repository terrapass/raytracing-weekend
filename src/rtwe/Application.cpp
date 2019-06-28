#include "Application.h"

#include <cassert>
#include <boost/log/trivial.hpp>

#include <sdl2utils/event_utils.h>
#include <sdl2utils/guards.h>

#include "constants.h"
#include "math_utils.h"
#include "tracing.h"
#include "targets.h"
#include "Camera.h"
#include "Image.h"
#include "threading.h"
#include "tasks.h"

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

static inline Color RawNormalToColor(const Vector3 & rawNormal);

static inline void InitSamplingThreadPool(
    RepeatingSamplePixelTask::SamplingThreadPool & threadPool,
    Image &                                        targetImage,
    const std::vector<Body> &                      scene,
    const Camera &                                 camera,
    const RayMissFunction                          rayMissFunc
);

int Application::run()
{
    const sdl2utils::SDL_WindowPtr window = createWindow();
    assert(window);

    const sdl2utils::SDL_RendererPtr renderer = createRenderer(window.get());
    assert(renderer);

    const sdl2utils::SDL_TexturePtr streamingTexture = createStreamingTexture(renderer.get());
    assert(streamingTexture);

    const std::vector<Body> raytracingScene = createRaytracingScene();

    static const float PROJECTION_HEIGHT = 2.0f;
    static const float PROJECTION_WIDTH  = PROJECTION_HEIGHT * WINDOW_ASPECT_RATIO;

    static const Color BACKGROUND_TOP_COLOR   (0.7f, 0.7f, 0.95f);
    static const Color BACKGROUND_BOTTOM_COLOR(0.9f, 0.9f, 0.9f);

    // The following code uses a left-handed coordinate system:
    // x points right, y points up, z points into the screen.

    static const Vector3 CAMERA_ORIGIN    (0.0f, 0.0f, -1.0f);
    static const Vector3 CAMERA_UP        (0.0f, 1.0f, 0.0f);
    static const Vector3 PROJECTION_CENTER(0.0f, 0.0f, 0.0f);

    const Camera camera(
        CAMERA_ORIGIN,
        PROJECTION_CENTER,
        CAMERA_UP,
        PROJECTION_WIDTH,
        PROJECTION_HEIGHT
    );

    const RayMissFunction rayMissFunc = std::bind(
        GetVerticalGradientColor,
        std::placeholders::_1,
        BACKGROUND_BOTTOM_COLOR,
        BACKGROUND_TOP_COLOR
    );

    Image raytracedImage(WINDOW_WIDTH, WINDOW_HEIGHT);

    RepeatingSamplePixelTask::SamplingThreadPool threadPool(HARDWARE_MAX_CONCURRENT_THREADS - 1);
    InitSamplingThreadPool(
        threadPool,
        raytracedImage,
        raytracingScene,
        camera,
        rayMissFunc
    );

    while (!sdl2utils::escOrCrossPressed())
    {
        void * pixels = nullptr;
        int    pitch  = -1;

        const int lockResult = SDL_LockTexture(streamingTexture.get(), nullptr, &pixels, &pitch);
        assert(lockResult == 0 && "SDL_LockTexture() must succeed");

        for (int y = 0; y < WINDOW_HEIGHT; y++)
        {
            for (int x = 0; x < WINDOW_WIDTH; x++)
            {
                const int      pixelOffset = y*pitch + x*sizeof(Uint32);
                Uint32 * const pixel       = reinterpret_cast<Uint32 *>(reinterpret_cast<Uint8 *>(pixels) + pixelOffset);

                *pixel = raytracedImage.GetPixelColor(x, y).ToArgb();
            }
        }

        SDL_UnlockTexture(streamingTexture.get());

        SDL_RenderCopy(renderer.get(), streamingTexture.get(), nullptr, nullptr);
        SDL_RenderPresent(renderer.get());
    }

    return 0;
}

//
// Service
//

static inline Color RawNormalToColor(const Vector3 & rawNormal)
{
    const Vector3 normal            = rawNormal.normalized();
    const Vector3 nonNegativeNormal = 0.5f*(normal + Vector3(1.0f, 1.0f, 1.0f));

    return Color(nonNegativeNormal);
}

static inline void InitSamplingThreadPool(
    RepeatingSamplePixelTask::SamplingThreadPool & threadPool,
    Image &                                        targetImage,
    const std::vector<Body> &                      scene,
    const Camera &                                 camera,
    const RayMissFunction                          rayMissFunc
)
{
    for (int y = 0; y < targetImage.GetHeight(); y++)
    {
        for (int x = 0; x < targetImage.GetWidth(); x++)
        {
            threadPool.EmplaceTask(
                threadPool,
                targetImage,
                scene,
                camera,
                rayMissFunc,
                x,
                y
            );
        }
    }
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

std::vector<Body> Application::createRaytracingScene()
{
    return {
        {
            std::make_shared<PlaneRayTarget>(Vector3(0.0f, -0.5f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
            Material{
                Color(0.75f, 0.75f, 0.75f),
                0.25f, 0.85f, 0.0f, 1.0f
            }
        },
        {
            std::make_shared<SphereRayTarget>(Vector3(0.0f, 0.0f, 1.0f), 0.5f),
            Material{
                Color(0.75f, 0.75f,  0.75f),
                0.975f, 0.975f, 0.975f, 1.5f
            }
        },
        {
            std::make_shared<SphereRayTarget>(Vector3(0.75f, -0.25f, 0.75f), 0.25f),
            Material{
                Color(0.35f, 0.7f, 0.35f),
                0.75f, 1.0f, 0.0f, 1.0f
            }
        },
        {
            std::make_shared<SphereRayTarget>(Vector3(-1.25f, 0.25f, 1.5f), 0.75f),
            Material{
                Color(0.8f, 0.4f, 0.6f),
                0.0f, 1.0f, 0.0f, 1.0f
            }
        }
    };
}

} // namespace rtwe