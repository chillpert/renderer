#include "base/Window.hpp"

namespace RENDERER_NAMESPACE
{
  Window::Window( int width, int height, const char* title, uint32_t flags ) :
    window( nullptr ),
    width( width ),
    height( height ),
    title( title ),
    flags( flags )
  {
    this->flags |= SDL_WINDOW_VULKAN;
  }

  Window::~Window( )
  {
    clean( );
  }

  void Window::init( )
  {
    SDL_SetHint( SDL_HINT_FRAMEBUFFER_ACCELERATION, "1" );

    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
      RX_FATAL( SDL_GetError( ) );

    this->window = SDL_CreateWindow( this->title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, this->width, this->height, this->flags );

    if ( this->window == nullptr )
      RX_FATAL( "Failed to create window." ); 
  }

  bool Window::update( )
  {
    // Updates local timer bound to this window.
    time.update( );
    
    // Fetch the latest window dimensions.
    int width, height;
    SDL_GetWindowSize( this->window, &width, &height );
    resize( width, height );

    return true;
  }

  void Window::clean( )
  {
    SDL_DestroyWindow( this->window );
    this->window = nullptr;

    SDL_Quit( );
  }

  void Window::resize( int width, int height )
  {
    this->width = width;
    this->height = height;

#if defined( _WIN32 ) || defined( _WIN64 )
    SDL_SetWindowSize( this->window, this->width, this->height );
#endif
  }

  std::vector<const char*> Window::getInstanceExtensions( )
  {
    uint32_t sdlExtensionsCount;
    SDL_bool result = SDL_Vulkan_GetInstanceExtensions( this->window, &sdlExtensionsCount, nullptr );

    if ( result != SDL_TRUE )
      RX_ERROR( "Failed to get extensions required by SDL." );

    const char** sdlExtensionsNames = new const char* [sdlExtensionsCount];
    result = SDL_Vulkan_GetInstanceExtensions( this->window, &sdlExtensionsCount, sdlExtensionsNames );

    if ( result != SDL_TRUE )
      RX_ERROR( "Failed to get extensions required by SDL." );

    std::vector<const char*> extensions;
    extensions.reserve( sdlExtensionsCount );

    for ( size_t i = 0; i < sdlExtensionsCount; ++i )
      extensions.push_back( sdlExtensionsNames[i] );

    return extensions;
  }

  vk::SurfaceKHR Window::createSurface( vk::Instance instance )
  {
    VkSurfaceKHR surface;
    SDL_bool result = SDL_Vulkan_CreateSurface( this->window, instance, &surface );

    if ( result != SDL_TRUE )
      RX_ERROR( "Failed to create surface" );

    return surface;
  }

  vk::Extent2D Window::getExtent( ) const
  {
    int width, height;
    SDL_GetWindowSize( this->window, &width, &height );

    return { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) };
  }

  bool Window::changed( )
  {
    static int prevWidth = this->width;
    static int prevHeight = this->height;

    if ( this->width != prevWidth || this->height != prevHeight )
    {
      prevWidth = this->width;
      prevHeight = this->height;
      return true;
    }

    return false;
  }

  bool Window::minimized( )
  {
    if ( SDL_GetWindowFlags( this->window ) & SDL_WINDOW_MINIMIZED )
      return true;

    return false;
  }
}