cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0048 NEW) # Set version strings from project() call
project(RayTracingWeekend VERSION 0.0.1)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define user options

if(NOT WIN32)
    option(RTWE_BOOST_LOG_DYN_LINK "Link dynamically to boost_log (define BOOST_LOG_DYN_LINK and Boost_USE_STATIC_LIBS)." OFF)
    mark_as_advanced(RTWE_BOOST_LOG_DYN_LINK)
else()
    set(RTWE_BOOST_LOG_DYN_LINK OFF)
endif()

# Setup paths to load cmake modules from
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Configure warnings on GCC and Clang
include(EnableCXXCompilerFlagIfSupported)
if(NOT MSVC)    # Don't pass /Wall to MSVC, as it would throw a fit and produce a ton of silly warnings in system headers
    enable_cxx_compiler_flag_if_supported("-Wall")
    enable_cxx_compiler_flag_if_supported("-Wextra")
    enable_cxx_compiler_flag_if_supported("-pedantic")
endif(NOT MSVC)

# Include dependencies

# sdl2utils
add_subdirectory("submodules/sdl2utils")

# Find external packages
set(SDL2_BUILDING_LIBRARY false)    # SDL2
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)   # SDL2_image
find_package(SDL2_ttf REQUIRED)     # SDL2_ttf
find_package(SDL2_mixer REQUIRED)   # SDL2_mixer

if(NOT RTWE_BOOST_LOG_DYN_LINK)
    set(Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost COMPONENTS log REQUIRED) # Boost.Log

# Select *.cpp files
file(
    GLOB_RECURSE
    RTWE_CPPS
    "src/rtwe/*.cpp"
)

# Use *.cpp and (optionally) *.cxx files as sources for RTWE
set(RTWE_SOURCES ${RTWE_CXXS} ${RTWE_CPPS})

# Setup include directories
include_directories(
    "src/rtwe"                  # own header files
    #"submodules/sdl2utils"     # sdl2utils
    "submodules/nlohmann_json/include" # nlohmann/json
    ${SDL2_INCLUDE_DIR}         # SDL2
    ${SDL2_IMAGE_INCLUDE_DIR}   # SDL2_image
    ${SDL2_TTF_INCLUDE_DIR}     # SDL2_ttf
    ${SDL2_MIXER_INCLUDE_DIRS}  # SDL2_mixer
    ${Boost_INCLUDE_DIR}        # Boost
)

# Setup link directories (only needed for MSBuild on Windows to be able to find Boost libs)
link_directories(
    ${Boost_LIBRARY_DIR_RELEASE} # Boost
)

# rtwe library to be linked into main and (optionally) test executables
add_library(
    rtwe
    ${RTWE_SOURCES}
)
target_link_libraries(
    rtwe
    sdl2utils
    ${SDL2_LIBRARY} 
    ${SDL2_IMAGE_LIBRARIES} 
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    Boost::log
)

# Main RayTracingWeekend executable
add_executable(
    rtwe_main
    "src/main.cpp"
)
target_link_libraries(
    rtwe_main
    rtwe
)
set_target_properties(rtwe_main PROPERTIES OUTPUT_NAME "rtwe")

# Define preprocessor symbols for both rtwe and rtwe_main targets

# If the corresponding cache entry is set to ON, define BOOST_LOG_DYN_LINK for linking against boost_log dynamically
if(RTWE_BOOST_LOG_DYN_LINK)
    message(STATUS "Will link dynamically against boost_log")
    target_compile_definitions(rtwe PUBLIC BOOST_LOG_DYN_LINK)
else()
    message(STATUS "Will link statically against boost_log")
endif()

# Also define SDL_MAIN_HANDLED for Windows build to fix unresolved reference linking error for main()
if(WIN32)
    target_compile_definitions(rtwe_main PRIVATE SDL_MAIN_HANDLED)
endif(WIN32)

# Copy assets/ directory after building rtwe executable
# Note: `cmake -E copy_directory ...` actually performs directory contents copy, 
# so it might be useful if it's needed in the future to copy the contents of assets/ into the build/install root,
# instead of the assets/ subdirectory.
add_custom_command(
    TARGET rtwe_main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:rtwe_main>/assets"
)

# Mark assets/ directory to be removed from build directory on `make clean`
set_directory_properties(
    PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "assets"
)

