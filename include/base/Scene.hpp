#ifndef SCENE_HPP
#define SCENE_HPP

#include "Settings.hpp"
#include "base/Geometry.hpp"
#include "base/Lights.hpp"

namespace RAYEXEC_NAMESPACE
{
  class Api;
  class RayExec;

  class Scene
  {
  public:
    friend Api;
    friend RayExec;

    /// @return Returns all geometry instances in the scene.
    RX_API auto getGeometryInstances( ) const -> const std::vector<std::shared_ptr<GeometryInstance>>&;

    /// Used to submit a geometry instance for rendering.
    /// @param geometryInstance The instance to queue for rendering.
    /// @note This function does not invoke any draw calls.
    RX_API void submitGeometryInstance( std::shared_ptr<GeometryInstance> geometryInstance );

    /// Used to submit multiple geometry instances for rendering, replacing all existing instances.
    /// @param geometryInstances The instances to queue for rendering.
    /// @note This function does not invoke any draw calls.
    RX_API void setGeometryInstances( const std::vector<std::shared_ptr<GeometryInstance>>& geometryInstances );

    /// @return Returns all directional lights in the scene.
    RX_API auto getDirectionalLights( ) const -> const std::vector<std::shared_ptr<DirectionalLight>>&;

    /// Used to submit a directional light.
    /// @param light The directional light to submit.
    RX_API void submitDirectionalLight( std::shared_ptr<DirectionalLight> light );

    /// Used to remove a directional light.
    /// @param light The directional light to remove.
    RX_API void removeDirectionalLight( std::shared_ptr<DirectionalLight> light );

    /// @return Returns all point lights in the scene.
    RX_API auto getPointLights( ) const -> const std::vector<std::shared_ptr<PointLight>>&;

    /// Used to submit a point light.
    /// @param light The point light to submit.
    RX_API void submitPointLight( std::shared_ptr<PointLight> light );

    /// Used to remove a point light.
    /// @param light The point light to remove.
    RX_API void removePointLight( std::shared_ptr<PointLight> light );

    /// Used to remove a geometry instance.
    ///
    /// Once a geometry instance was removed, it will no longer be rendered.
    /// @param geometryInstance The instance to remove.
    RX_API void removeGeometryInstance( std::shared_ptr<GeometryInstance> geometryInstance );

    /// Used to submit a geometry and set up its buffers.
    ///
    /// Once a geometry was submitted, geometry instances referencing this particular geometry can be drawn.
    /// @param geometry The geometry to submit.
    RX_API void submitGeometry( std::shared_ptr<Geometry> geometry );

    /// Used to submit multiple geometries and set up their buffers.
    ///
    /// Once a geometry was submitted, geometry instances referencing this particular geometry can be drawn.
    /// @param geometries The geometries to submit.
    RX_API void setGeometries( const std::vector<std::shared_ptr<Geometry>>& geometries );

    /// Used to retrieve a geoemtry based on its path.
    /// @param path The geometry's model's path, relative to the path to assets.
    RX_API auto findGeometry( std::string_view path ) const -> std::shared_ptr<Geometry>;

  private:
    std::vector<std::shared_ptr<Geometry>> geometries;                ///< Stores all geometries.
    std::vector<std::shared_ptr<GeometryInstance>> geometryInstances; ///< Stores all geometry instances.
    std::vector<std::shared_ptr<DirectionalLight>> directionalLights; ///< Stores all directional lights.
    std::vector<std::shared_ptr<PointLight>> pointLights;             ///< Stores all point lights.

    bool uploadGeometryInstancesToBuffer = false; ///< Keeps track of whether or not to upload the ray tracing instances to their respective buffer the next time update() is called.
    bool uploadDirectionalLightsToBuffer = false; ///< Keeps track of whether or not to upload the directional lights to their respective buffer the next time update() is called.
    bool uploadPointLightsToBuffer       = false; ///< Keeps track of whether or not to upload the point lights to their respective buffer the next time update() is called.

    Settings* settings = nullptr; ///< Refers to the rendering settings stored in RayExec::settings.
  };
} // namespace RAYEXEC_NAMESPACE

#endif // SCENE_HPP
