#include <sdl2utils/raii.h>

// Forward declarations
namespace sdl2utils::raii
{

class ScopedSDLCore;

} // namespace sdl2utils::raii

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

private: // Constants

    static const int SDL_INIT_FLAGS;

private: // Members

    const sdl2utils::raii::ScopedSDLCore m_ScopedSDLCore;
};

} // namespace rtwe
