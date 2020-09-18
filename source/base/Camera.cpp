#include "base/Camera.hpp"

namespace RENDERER_NAMESPACE
{
  Camera::Camera( int width, int height, const glm::vec3& position ) :
    width( width ),
    height( height ),
    position( position )
  {
    updateVectors( );
    updateViewMatrix( );
  }

  void Camera::setPosition( const glm::vec3& position )
  {
    this->position = position;

    updateViewMatrix( );
  }

  void Camera::setSize( int width, int height )
  {
    this->width = width;
    this->height = height;

    updateProjectionMatrix( );
  }

  void Camera::setFov( float fov )
  {
    this->fov = fov;

    updateProjectionMatrix( );
  }

  void Camera::setSensitivity( float sensitivity )
  {
    this->sensitivity = sensitivity;
  }

  void Camera::updateViewMatrix( )
  {
    this->view = glm::lookAt( this->position, this->position + this->front, this->worldUp );

    this->viewInverse = glm::inverse( this->view );
  }

  void Camera::updateProjectionMatrix( )
  {
    this->projection = glm::perspective( glm::radians( this->fov ), static_cast<float>( this->width ) / static_cast<float>( this->height ), 0.1f, 100.0f );
    this->projection[1, 1] *= -1;

    this->projectionInverse = glm::inverse( this->projection );
  }

  void Camera::processMouse( float xOffset, float yOffset )
  {
    this->updateView = true;

    xOffset *= this->sensitivity;
    yOffset *= this->sensitivity;

    this->yaw += xOffset;
    this->pitch += yOffset;

    if ( this->yaw > 360.0f )
      this->yaw = fmod( this->yaw, 360.0f );

    if ( this->yaw < 0.0f )
      this->yaw = 360.0f + fmod( this->yaw, 360.0f );

    if ( this->pitch > 89.0f )
      this->pitch = 89.0f;

    if ( this->pitch < -89.0f )
      this->pitch = -89.0f;

    updateVectors( );
  }

  void Camera::updateVectors( )
  {
    glm::vec3 t_front;
    t_front.x = cos( glm::radians( this->yaw ) ) * cos( glm::radians( this->pitch ) );
    t_front.y = sin( glm::radians( this->pitch ) );
    t_front.z = sin( glm::radians( this->yaw ) ) * cos( glm::radians( this->pitch ) );

    this->front = glm::normalize( t_front );
    this->right = glm::normalize( glm::cross( this->front, this->worldUp ) );
    this->up = glm::normalize( glm::cross( this->right, this->front ) );
  }
}