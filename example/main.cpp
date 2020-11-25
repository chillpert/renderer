#include "CustomCamera.hpp"
#include "CustomGui.hpp"
#include "CustomWindow.hpp"

auto main( ) -> int
{
  // Window dimensions.
  const int width  = 1400;
  const int height = 900;

  rx::Rayex renderer;

  // Custom camera
  renderer.setCamera( std::make_shared<CustomCamera>( width, height, glm::vec3( 0.0F, 0.0F, 3.0F ) ) );

  // Custom window
  renderer.setWindow( std::make_shared<CustomWindow>( width, height, "Rayex Example", SDL_WINDOW_RESIZABLE, renderer.getCamera( ) ) );

  // Custom ImGui based Gui
  renderer.setGui( std::make_shared<CustomGui>( &renderer ) );

  // Use resources efficiently by introducing the renderer to the anticipated total amount of various entities.
  renderer.settings( ).setMaxDirectionalLights( 2 );
  renderer.settings( ).setMaxPointLights( 20 );
  renderer.settings( ).setMaxGeometryInstances( 100 );
  renderer.settings( ).setMaxGeoemtry( 8 );

  // ... and initialize the renderer.
  renderer.init( );

  // Load geometries.
  auto awp   = rx::loadObj( "models/awpdlore/awpdlore.obj" );
  auto plane = rx::loadObj( "models/plane.obj" );

  plane->meshes[0].material.diffuseTexPath = "models/metal.png";

  // Submit geometries.
  renderer.scene( ).setGeometries( { awp, plane } );

  // Create instances of the geometries.
  auto transform = glm::scale( glm::mat4( 1.0F ), glm::vec3( 0.25F ) );
  transform      = glm::rotate( transform, glm::radians( 45.0F ), glm::vec3( 0.0F, 1.0F, 0.0F ) );
  transform      = glm::translate( transform, glm::vec3( 0.0F, -1.0F, 0.5F ) );

  auto awpInstance1 = rx::instance( awp, transform );

  transform = glm::scale( glm::mat4( 1.0F ), glm::vec3( 0.25F ) );
  transform = glm::rotate( transform, glm::radians( 90.0F ), glm::vec3( 0.0F, 1.0F, 0.0F ) );
  transform = glm::translate( transform, glm::vec3( 1.0F, 2.0F, 0.0F ) );

  auto awpInstance2 = rx::instance( awp, transform );

  transform = glm::scale( glm::mat4( 1.0F ), glm::vec3( 0.1F ) );
  transform = glm::translate( transform, glm::vec3( 0.0F, -80.0F, 0.0F ) );

  auto planeInstance = rx::instance( plane, transform );

  // Submit instances for drawing.
  renderer.scene( ).setGeometryInstances( { awpInstance1, planeInstance, awpInstance2 } );

  // Submit lights.
  auto directionalLight = rx::directionalLightInstance( glm::vec3( -4.0F, 10.0F, 5.0F ) );

  renderer.scene( ).submitDirectionalLight( directionalLight );

  renderer.scene( ).setSkybox( "models/skybox/cubemap_yokohama_bc3_unorm.ktx" );

  while ( renderer.isRunning( ) )
  {
    renderer.run( );

    awpInstance1->setTransform( glm::rotate( awpInstance1->transform, rx::Time::getDeltaTime( ) * 0.5F, glm::vec3( 0.0F, 1.0F, 0.0F ) ) );

    // Extra tests for memcpy error: (hold to spawn many boxes at once)
    if ( Key::eB )
    {
      addBox( &renderer );
    }

    if ( Key::eL )
    {
      addSphere( &renderer );
    }
  }

  return 0;
}
