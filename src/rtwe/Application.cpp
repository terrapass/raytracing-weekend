#include "Application.h"

#include <cassert>

#include <sdl2utils/event_utils.h>
#include <sdl2utils/guards.h>

namespace rtwe
{

//
// Constants
//

const int Application::SDL_INIT_FLAGS = SDL_INIT_EVERYTHING;

const int Application::WINDOW_WIDTH  = 800;
const int Application::WINDOW_HEIGHT = 600;

const char * const Application::WINDOW_TITLE = "Ray Tracing Weekend";

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

sdl2utils::SDL_RendererPtr Application::createRenderer(SDL_Window * const pWindow)
{
    return sdl2utils::SDL_RendererPtr(
        sdl2utils::guards::ensureNotNull(
            SDL_CreateRenderer(pWindow, -1, 0),
            "result of SDL_CreateRenderer()"
        )
    );
}

} // namespace rtwe