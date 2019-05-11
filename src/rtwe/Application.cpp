#include "Application.h"

namespace rtwe
{

//
// Constants
//

const int Application::SDL_INIT_FLAGS = SDL_INIT_EVERYTHING;

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
    // TODO

    return 0;
}

} // namespace rtwe