#include "RayExec.hpp"

using namespace rx;

float animationSpeed = 0.0F;

namespace Key
{
  bool eW;
  bool eA;
  bool eS;
  bool eD;
  bool eC;
  bool eSpace;
  bool eLeftShift;
} // namespace Key

class CustomCamera : public Camera
{
public:
  CustomCamera( int width, int height, const glm::vec3& position ) :
    Camera( width, height, position ) {}

  void update( ) override
  {
    Camera::update( );

    processKeyboard( );
  }

  void processKeyboard( )
  {
    const float defaultSpeed  = 2.5F;
    static float currentSpeed = defaultSpeed;
    float finalSpeed          = currentSpeed * Time::getDeltaTime( );

    if ( Key::eLeftShift )
    {
      currentSpeed = 10.0F;
    }
    else
    {
      currentSpeed = defaultSpeed;
    }

    if ( Key::eW )
    {
      this->position += this->front * finalSpeed;
      this->updateView = true;
    }

    if ( Key::eS )
    {
      this->position -= this->front * finalSpeed;
      this->updateView = true;
    }

    if ( Key::eA )
    {
      this->position -= this->right * finalSpeed;
      this->updateView = true;
    }

    if ( Key::eD )
    {
      this->position += this->right * finalSpeed;
      this->updateView = true;
    }

    if ( Key::eC )
    {
      this->position.y -= finalSpeed / 2.0F;
      this->updateView = true;
    }

    if ( Key::eSpace )
    {
      this->position.y += finalSpeed / 2.0F;
      this->updateView = true;
    }

    updateViewMatrix( );
  }
};

class CustomWindow : public Window
{
public:
  CustomWindow( int width, int height, const char* title, uint32_t flags, std::shared_ptr<Camera> camera ) :
    Window( width, height, title, flags ),
    camera( std::move( camera ) )
  {
  }

  auto init( ) -> bool override
  {
    if ( !Window::init( ) )
    {
      return false;
    }

    SDL_SetRelativeMouseMode( SDL_FALSE );
    return true;
  }

  auto update( ) -> bool override
  {
    if ( !Window::update( ) )
    {
      return false;
    }

    this->camera->setSize( this->width, this->height );

    // Add your custom event polling and integrate your event system.
    SDL_Event event;

    while ( SDL_PollEvent( &event ) != 0 )
    {
      switch ( event.type )
      {
        case SDL_QUIT: {
          return false;
        }

        case SDL_WINDOWEVENT: {
          switch ( event.window.event )
          {
            case SDL_WINDOWEVENT_CLOSE:
              return false;

            case SDL_WINDOWEVENT_RESIZED:
              resize( static_cast<int>( event.window.data1 ), static_cast<int>( event.window.data2 ) );
              break;

            case SDL_WINDOWEVENT_MINIMIZED:
              resize( 0, 0 );
              break;
          }
          break;
        }

        case SDL_KEYDOWN: {
          switch ( event.key.keysym.sym )
          {
            case SDLK_w:
              Key::eW = true;
              break;

            case SDLK_a:
              Key::eA = true;
              break;

            case SDLK_s:
              Key::eS = true;
              break;

            case SDLK_d:
              Key::eD = true;
              break;

            case SDLK_LSHIFT:
              Key::eLeftShift = true;
              break;

            case SDLK_ESCAPE:
              return false;

            case SDLK_SPACE: {
              if ( this->mouseVisible )
              {
                this->mouseVisible = false;
                SDL_SetRelativeMouseMode( SDL_TRUE );
                SDL_GetRelativeMouseState( nullptr, nullptr ); // Magic fix!
              }
              else
              {
                SDL_SetRelativeMouseMode( SDL_FALSE );
                this->mouseVisible = true;
              }

              break;
            }
          }
          break;
        }

        case SDL_KEYUP: {
          switch ( event.key.keysym.sym )
          {
            case SDLK_w:
              Key::eW = false;
              break;

            case SDLK_a:
              Key::eA = false;
              break;

            case SDLK_s:
              Key::eS = false;
              break;

            case SDLK_d:
              Key::eD = false;
              break;

            case SDLK_LSHIFT:
              Key::eLeftShift = false;
              break;
          }
          break;
        }

        case SDL_MOUSEMOTION: {
          if ( !this->mouseVisible )
          {
            int x;
            int y;
            SDL_GetRelativeMouseState( &x, &y );
            this->camera->processMouse( x, -y );
            break;
          }
        }
      }
    }
    return true;
  }

private:
  std::shared_ptr<Camera> camera;
  bool mouseVisible = true;
};

class CustomGui : public Gui
{
public:
  CustomGui( RayExec* renderer ) :
    renderer( renderer ) {}

private:
  void configure( ) override
  {
    Gui::configure( );
    ImGui::StyleColorsDark( );
  }

  void render( ) override
  {
    static bool showDemoWindow = false;
    if ( showDemoWindow )
      ImGui::ShowDemoWindow( );

    if ( ImGui::Begin( "Settings" ) )
    {
      //ImGui::SliderFloat( "Speed", &animationSpeed, 0.0F, 2.0F );

      if ( ImGui::Button( "Add Box" ) )
      {
        int max = 10;
        int min = -10;
        srand( time( nullptr ) );
        int x = rand( ) % ( max - min + 1 ) + min;
        int y = rand( ) % ( max - min + 1 ) + min;
        int z = rand( ) % ( max - min + 1 ) + min;

        auto box            = std::make_shared<GeometryNode>( "models/cube.obj", Material( "textures/container.png" ) );
        box->worldTransform = glm::translate( box->worldTransform, glm::vec3( x, y, z ) );
        box->worldTransform = glm::scale( box->worldTransform, glm::vec3( 0.3F, 0.3F, 0.3F ) );
        this->renderer->pushNode( box );

        this->geometryNodes.push_back( box );
      }

      ImGui::SameLine( );

      if ( ImGui::Button( "Add Sphere" ) )
      {
        int max = 10;
        int min = -10;
        srand( time( nullptr ) );
        int x = rand( ) % ( max - min + 1 ) + min;
        int y = rand( ) % ( max - min + 1 ) + min;
        int z = rand( ) % ( max - min + 1 ) + min;

        auto sphere            = std::make_shared<GeometryNode>( "models/sphere.obj", Material( "textures/metal.png" ) );
        sphere->worldTransform = glm::translate( sphere->worldTransform, glm::vec3( x, y, z ) );
        sphere->worldTransform = glm::scale( sphere->worldTransform, glm::vec3( 0.3F, 0.3F, 0.3F ) );
        this->renderer->pushNode( sphere );

        this->geometryNodes.push_back( sphere );
      }

      ImGui::SameLine( );

      if ( ImGui::Button( "Add directional light" ) )
      {
        auto dirLight = std::make_shared<DirectionalLightNode>( );

        int max = 4;
        int min = -4;
        srand( time( nullptr ) );
        int finalNum             = rand( ) % ( max - min + 1 ) + min;
        dirLight->worldTransform = glm::translate( dirLight->worldTransform, glm::vec3( finalNum, 0.0F, 0.0F ) );
      }

      ImGui::SameLine( );

      if ( ImGui::Button( "Clear scene" ) )
      {
        for ( const auto& node : this->geometryNodes )
        {
          this->renderer->popNode( node );
        }
      }

      ImGui::Checkbox( "Show ImGui Demo Window", &showDemoWindow );

      auto clearColor = this->renderer->settings.getClearColor( );
      if ( ImGui::ColorEdit4( "##AmbientColor", &clearColor[0] ) )
      {
        this->renderer->settings.setClearColor( clearColor );
      }

      bool rayTrace = this->renderer->settings.getRayTracingEnabled( );
      if ( ImGui::Checkbox( "Toggle Ray Tracing", &rayTrace ) )
      {
        this->renderer->settings.setEnableRayTracing( rayTrace );
        if ( !rayTrace )
        {
          this->renderer->settings.setEnableJitterCam( false );
        }
      }

      if ( rayTrace )
      {
        bool jitterCamEnabled = this->renderer->settings.getJitterCamEnabled( );
        if ( ImGui::Checkbox( "Toggle Jitter Cam", &jitterCamEnabled ) )
        {
          this->renderer->settings.setEnableJitterCam( jitterCamEnabled );
        }

        if ( jitterCamEnabled )
        {
          int jitterCamSampleRate = static_cast<int>( this->renderer->settings.getJitterCamSampleRate( ) );
          if ( ImGui::SliderInt( "Set Jitter Cam Sample Rate", &jitterCamSampleRate, 1, 100 ) )
          {
            this->renderer->settings.setJitterCamSampleRate( jitterCamSampleRate );
          }

          int jitterCamSampleRatePerRayGen = static_cast<int>( this->renderer->settings.getJitterCamSampleRatePerRayGen( ) );
          if ( ImGui::SliderInt( "Set Jitter Cam Sample Rate Per Ray Gen", &jitterCamSampleRatePerRayGen, 1, 10 ) )
          {
            this->renderer->settings.setJitterCamSampleRatePerRayGen( jitterCamSampleRatePerRayGen );
          }
        }

        bool msaaEnabled = this->renderer->settings.getMsaaEnabled( );
        if ( ImGui::Checkbox( "Toggle MSAA", &msaaEnabled ) )
        {
          this->renderer->settings.setEnableMsaa( msaaEnabled );
        }

        if ( msaaEnabled )
        {
          static int msaaSampleRate = static_cast<int>( this->renderer->settings.getMsaaSampleRate( ) );
          if ( ImGui::RadioButton( "2x", &msaaSampleRate, 2 ) )
          {
            this->renderer->settings.setMsaaSampleRate( 2 );
          }

          ImGui::SameLine( );

          if ( ImGui::RadioButton( "4x", &msaaSampleRate, 4 ) )
          {
            this->renderer->settings.setMsaaSampleRate( 4 );
          }

          ImGui::SameLine( );

          if ( ImGui::RadioButton( "8x", &msaaSampleRate, 8 ) )
          {
            this->renderer->settings.setMsaaSampleRate( 8 );
          }

          ImGui::SameLine( );

          if ( ImGui::RadioButton( "16x", &msaaSampleRate, 16 ) )
          {
            this->renderer->settings.setMsaaSampleRate( 16 );
          }
        }

        int depth = static_cast<int>( this->renderer->settings.getRecursionDepth( ) );
        if ( ImGui::SliderInt( "Recursion depth", &depth, 0, 31 ) )
        {
          this->renderer->settings.setRecursionDepth( static_cast<uint32_t>( depth ) );
        }
      }

      const size_t maxFrames = 10000;
      static std::array<float, maxFrames> frameTimes;

      static size_t counter = 0;
      counter               = counter % maxFrames;

      float dt = Time::getDeltaTime( );
      if ( dt > 0.001f )
      {
        frameTimes[counter] = dt;
        ++counter;
      }

      ImGui::PlotLines( "Frame Times", frameTimes.data( ), maxFrames, 0, "Frametime", 0.0F, 0.01F, ImVec2( 0.0F, 80.0F ) );
    }

    ImGui::End( );
  }

private:
  RayExec* renderer;
  std::vector<std::shared_ptr<GeometryNode>> geometryNodes;
};

auto main( ) -> int
{
  // Window dimensions.
  const int width  = 1400;
  const int height = 900;

  RayExec renderer;

  // Custom camera
  renderer.setCamera( std::make_shared<CustomCamera>( width, height, glm::vec3( 0.0F, 0.0F, 3.0F ) ) );

  // Custom window
  renderer.setWindow( std::make_shared<CustomWindow>( width, height, "Example", SDL_WINDOW_RESIZABLE, renderer.getCamera( ) ) );

  // Custom ImGui based Gui
  renderer.setGui( std::make_shared<CustomGui>( &renderer ) );

  // Use resources efficiently by introducing the renderer to the anticipated total amount of various entities.
  renderer.settings.setMaxDirectionalLights( 5 );
  renderer.settings.setMaxPointLights( 0 ); // Bad input: 1 will be used instead (See warning in console).
  renderer.settings.setMaxGeometryNodes( 50 );

  // ... and initialize the renderer.
  renderer.init( );

  renderer.setModels( { "models/plane.obj", "models/sphere.obj", "models/awpdlore/awpdlore.obj", "models/cube.obj" } );

  // Setup the scene
  auto dragonLore            = std::make_shared<GeometryNode>( "models/awpdlore/awpdlore.obj", Material( "textures/awpdlore.png" ) );
  dragonLore->worldTransform = glm::scale( dragonLore->worldTransform, glm::vec3( 0.25F ) );
  dragonLore->worldTransform = glm::rotate( dragonLore->worldTransform, glm::radians( 45.0F ), glm::vec3( 0.0F, 1.0F, 0.0F ) );
  dragonLore->worldTransform = glm::translate( dragonLore->worldTransform, glm::vec3( 0.0F, -1.0F, 0.5F ) );

  auto dragonLore2            = std::make_shared<GeometryNode>( "models/awpdlore/awpdlore.obj", Material( "textures/awpdlore.png" ) );
  dragonLore2->worldTransform = glm::scale( dragonLore2->worldTransform, glm::vec3( 0.25F ) );
  dragonLore2->worldTransform = glm::rotate( dragonLore2->worldTransform, glm::radians( 90.0F ), glm::vec3( 0.0F, 1.0F, 0.0F ) );
  dragonLore2->worldTransform = glm::translate( dragonLore2->worldTransform, glm::vec3( 1.0F, 2.0F, 0.0F ) );

  auto floor = std::make_shared<GeometryNode>( "models/plane.obj", Material( "textures/awpdlore.png" ) );

  auto directionalLight = std::make_shared<DirectionalLightNode>( );

  // Add the model to the renderer. This way they will be queued for rendering.
  renderer.pushNode( dragonLore );
  renderer.pushNode( dragonLore2 );
  renderer.pushNode( floor );
  renderer.pushNode( directionalLight );

  while ( renderer.isRunning( ) )
  {
    //dragonLore->worldTransform = glm::rotate( dragonLore->worldTransform, glm::radians( 90.0F ) * Time::getDeltaTime( ) * animationSpeed, glm::vec3( 0.0F, 1.0F, 0.0F ) );
    renderer.run( );
  }

  return 0;
}
