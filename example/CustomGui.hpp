#ifndef CUSTOM_GUI_HPP
#define CUSTOM_GUI_HPP

#include "Rayex.hpp"

class CustomGui : public rx::Gui
{
public:
  CustomGui( rx::Rayex* renderer ) :
    _renderer( renderer ) {}

private:
  void configure( ) override
  {
    rx::Gui::configure( );
    ImGui::StyleColorsDark( );
  }

  void render( ) override
  {
    static bool showDemoWindow = false;
    if ( showDemoWindow )
    {
      ImGui::ShowDemoWindow( );
    }

    if ( ImGui::Begin( "Settings" ) )
    {
      if ( ImGui::Button( "Add box" ) )
      {
        auto cube      = _renderer->scene( ).findGeometry( "models/cube.obj" );
        auto transform = glm::scale( glm::mat4( 1.0F ), glm::vec3( 0.3F, 0.3F, 0.3F ) );
        transform      = glm::rotate( transform, getRandomFloat( 0.0F, 360.0F ), glm::vec3( 0.0F, 1.0F, 0.0F ) );
        transform      = glm::translate( transform, getRandomUniquePosition( -10.0F, 10.0F ) );

        auto cubeInstance = rx::instance( cube, transform );
        _renderer->scene( ).submitGeometryInstance( cubeInstance );
      }

      ImGui::SameLine( );

      if ( ImGui::Button( "Add sphere" ) )
      {
        auto sphere    = _renderer->scene( ).findGeometry( "models/sphere.obj" );
        auto transform = glm::scale( glm::mat4( 1.0F ), glm::vec3( 0.1F, 0.1F, 0.1F ) );
        transform      = glm::rotate( transform, getRandomFloat( 0.0F, 360.0F ), glm::vec3( 0.0F, 1.0F, 0.0F ) );
        transform      = glm::translate( transform, getRandomUniquePosition( -45.0F, 45.0F ) );

        auto sphereInstance = rx::instance( sphere, transform );
        _renderer->scene( ).submitGeometryInstance( sphereInstance );
      }

      ImGui::SameLine( );

      if ( ImGui::Button( "Add awp" ) )
      {
        auto awp       = _renderer->scene( ).findGeometry( "models/awpdlore/awpdlore.obj" );
        auto transform = glm::scale( glm::mat4( 1.0F ), glm::vec3( 0.3F, 0.3F, 0.3F ) );
        transform      = glm::rotate( transform, getRandomFloat( 0.0F, 360.0F ), glm::vec3( 0.0F, 1.0F, 0.0F ) );
        transform      = glm::translate( transform, getRandomUniquePosition( -10.0F, 10.0F ) );

        auto awpInstance = rx::instance( awp, transform );
        _renderer->scene( ).submitGeometryInstance( awpInstance );
      }

      ImGui::SameLine( );

      if ( ImGui::Button( "Clear scene" ) )
      {
        auto geometryInstances = _renderer->scene( ).getGeometryInstances( );

        for ( auto geometryInstance : geometryInstances )
        {
          _renderer->scene( ).removeGeometryInstance( geometryInstance );
        }
      }

      if ( ImGui::Button( "Add directional light" ) )
      {
        auto directionalLight = rx::directionalLightInstance( getRandomUniquePosition( 5.0F, 10.0F ) );
        directionalLight->direction.x *= -1;

        _renderer->scene( ).submitDirectionalLight( directionalLight );
      }

      ImGui::SameLine( );

      if ( ImGui::Button( "Remove directional lights" ) )
      {
        auto directionalLights = _renderer->scene( ).getDirectionalLights( );

        for ( auto directionalLight : directionalLights )
        {
          _renderer->scene( ).removeDirectionalLight( directionalLight );
        }
      }

      ImGui::Checkbox( "Show ImGui demo window", &showDemoWindow );

      auto clearColor = _renderer->settings( ).getClearColor( );
      if ( ImGui::ColorEdit4( "##AmbientColor", &clearColor[0] ) )
      {
        _renderer->settings( ).setClearColor( clearColor );
      }

      bool rayTrace = _renderer->settings( ).getRayTracingEnabled( );
      if ( ImGui::Checkbox( "Toggle ray tracing", &rayTrace ) )
      {
        _renderer->settings( ).setEnableRayTracing( rayTrace );
        if ( !rayTrace )
        {
          _renderer->settings( ).setEnableJitterCam( false );
        }
      }

      if ( rayTrace )
      {
        bool jitterCamEnabled = _renderer->settings( ).getJitterCamEnabled( );
        if ( ImGui::Checkbox( "Toggle jitter cam", &jitterCamEnabled ) )
        {
          _renderer->settings( ).setEnableJitterCam( jitterCamEnabled );
        }

        if ( jitterCamEnabled )
        {
          int jitterCamSampleRate = static_cast<int>( _renderer->settings( ).getJitterCamSampleRate( ) );
          if ( ImGui::SliderInt( "Set jitter cam sample rate", &jitterCamSampleRate, 1, 100 ) )
          {
            _renderer->settings( ).setJitterCamSampleRate( jitterCamSampleRate );
          }

          int jitterCamSampleRatePerRayGen = static_cast<int>( _renderer->settings( ).getJitterCamSampleRatePerRayGen( ) );
          if ( ImGui::SliderInt( "Set jitter cam sample rate per ray gen", &jitterCamSampleRatePerRayGen, 1, 10 ) )
          {
            _renderer->settings( ).setJitterCamSampleRatePerRayGen( jitterCamSampleRatePerRayGen );
          }
        }

        bool ssaaEnabled = _renderer->settings( ).getSsaaEnabled( );
        if ( ImGui::Checkbox( "Toggle SSAA", &ssaaEnabled ) )
        {
          _renderer->settings( ).setEnableSsaa( ssaaEnabled );
        }

        if ( ssaaEnabled )
        {
          int ssaaSampleRate = static_cast<int>( _renderer->settings( ).getSsaaSampleRate( ) );
          if ( ImGui::SliderInt( "Set SSAA sample rate", &ssaaSampleRate, 1, 4 ) )
          {
            _renderer->settings( ).setSsaaSampleRate( ssaaSampleRate );
          }
        }

        int depth = static_cast<int>( _renderer->settings( ).getRecursionDepth( ) );
        if ( ImGui::SliderInt( "Recursion depth", &depth, 0, 31 ) )
        {
          _renderer->settings( ).setRecursionDepth( static_cast<uint32_t>( depth ) );
        }
      }

      const size_t maxFrames = 10000;
      static std::array<float, maxFrames> frameTimes;

      static size_t counter = 0;
      counter               = counter % maxFrames;

      float dt = rx::Time::getDeltaTime( );
      if ( dt > 0.001f )
      {
        if ( counter >= maxFrames - 1 )
        {
          std::fill( frameTimes.begin( ), frameTimes.end( ), 0.0f );
        }

        frameTimes[counter] = dt;
        ++counter;
      }

      ImGui::PlotLines( "Frametimes", frameTimes.data( ), maxFrames, 0, "Frametime", 0.0F, 0.01F, ImVec2( 0.0F, 80.0F ) );
    }

    ImGui::End( );
  }

  glm::vec3 getRandomUniquePosition( float min, float max )
  {
    static std::vector<glm::vec3> positions;

    static std::random_device rd;
    static std::mt19937 mt( rd( ) );
    std::uniform_real_distribution<float> dist( min, max );

    glm::vec3 result = glm::vec3( 0.0F );

    while ( true )
    {
      result.x = dist( mt );
      result.y = dist( mt );
      result.z = dist( mt );

      bool accepted = true;
      for ( const auto& position : positions )
      {
        if ( result == position )
        {
          accepted = false;
        }
      }

      if ( accepted )
      {
        break;
      }
    };

    return result;
  }

  float getRandomFloat( float min, float max )
  {
    static std::random_device rd;
    static std::mt19937 mt( rd( ) );
    std::uniform_real_distribution<float> dist( min, max );

    return dist( mt );
  }

private:
  rx::Rayex* _renderer;
};

#endif // CUSTOM_GUI_HPP
