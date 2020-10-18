#ifndef CUSTOM_GUI_HPP
#define CUSTOM_GUI_HPP

#include "RayExec.hpp"

class CustomGui : public rx::Gui
{
public:
  CustomGui( rx::RayExec* renderer ) :
    renderer( renderer ) {}

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
      if ( ImGui::Button( "Add Box" ) )
      {
        auto cube      = this->renderer->findGeometry( "models/cube.obj" );
        auto transform = glm::translate( glm::mat4( 1.0F ), getRandomUniquePosition( -5.0F, 5.0F ) );
        transform      = glm::scale( transform, glm::vec3( 0.3F, 0.3F, 0.3F ) );

        auto cubeInstance = instance( cube, transform );

        this->renderer->submitGeometryInstance( cubeInstance );
        this->geometryInstances.push_back( cubeInstance );
      }

      ImGui::SameLine( );

      if ( ImGui::Button( "Add Sphere" ) )
      {
        auto sphere    = this->renderer->findGeometry( "models/sphere.obj" );
        auto transform = glm::translate( glm::mat4( 1.0F ), getRandomUniquePosition( -5.0F, 5.0F ) );
        transform      = glm::scale( transform, glm::vec3( 0.3F, 0.3F, 0.3F ) );

        auto sphereInstance = instance( sphere, transform );
        this->renderer->submitGeometryInstance( sphereInstance );

        this->geometryInstances.push_back( sphereInstance );
      }

      ImGui::SameLine( );

      if ( ImGui::Button( "Clear scene" ) )
      {
        for ( auto geometryInstance : this->geometryInstances )
        {
          this->renderer->removeGeometryInstance( geometryInstance );
        }

        this->geometryInstances.clear( );
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

        bool ssaaEnabled = this->renderer->settings.getSsaaEnabled( );
        if ( ImGui::Checkbox( "Toggle SSAA", &ssaaEnabled ) )
        {
          this->renderer->settings.setEnableSsaa( ssaaEnabled );
        }

        if ( ssaaEnabled )
        {
          int ssaaSampleRate = static_cast<int>( this->renderer->settings.getSsaaSampleRate( ) );
          if ( ImGui::SliderInt( "Set SSAA Sample Rate", &ssaaSampleRate, 1, 4 ) )
          {
            this->renderer->settings.setSsaaSampleRate( ssaaSampleRate );
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

      ImGui::PlotLines( "Frame Times", frameTimes.data( ), maxFrames, 0, "Frametime", 0.0F, 0.01F, ImVec2( 0.0F, 80.0F ) );
    }

    ImGui::End( );
  }

  glm::vec3 getRandomUniquePosition( float min, float max )
  {
    static std::vector<glm::vec3> positions;

    static std::random_device rd;
    static std::mt19937 mt( rd( ) );
    static std::uniform_real_distribution<float> dist( min, max );

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

private:
  rx::RayExec* renderer;
  std::vector<std::shared_ptr<rx::GeometryInstance>> geometryInstances;
};

#endif // CUSTOM_GUI_HPP