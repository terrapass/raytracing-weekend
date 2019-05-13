#include <sdl2utils/raii.h>
#include <sdl2utils/pointers.h>

//
// Forward declarations
//

struct SDL_Window;

namespace sdl2utils::raii
{

class ScopedSDLCore;

} // namespace sdl2utils::raii

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

private: // Constants

    static const int SDL_INIT_FLAGS;

    static const int WINDOW_WIDTH;
    static const int WINDOW_HEIGHT;

    static const char * const WINDOW_TITLE;

private: // Members

    const sdl2utils::raii::ScopedSDLCore m_ScopedSDLCore;
};

} // namespace rtwe
