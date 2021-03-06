#pragma once

#include "base/Camera.hpp"
#include "base/Geometry.hpp"
#include "base/Settings.hpp"

namespace RAYEX_NAMESPACE
{
  class Api;
  class Rayex;

  /// Stores all geoemtry, geometry instances and light sources.
  /// Provides functions to change said data.
  /// @todo removeGeometry()
  /// @ingroup BASE
  /// @ingroup API
  class RX_API Scene
  {
  public:
    friend Api;
    friend Rayex;

    /// @return Returns all geometry instances in the scene.
    auto getGeometryInstances( ) const -> const std::vector<std::shared_ptr<GeometryInstance>>&;

    auto getGeometryInstance( size_t index ) const -> std::shared_ptr<GeometryInstance>;

    /// Used to submit a geometry instance for rendering.
    /// @param geometryInstance The instance to queue for rendering.
    /// @note This function does not invoke any draw calls.
    void submitGeometryInstance( std::shared_ptr<GeometryInstance> geometryInstance );

    /// Used to submit multiple geometry instances for rendering, replacing all existing instances.
    /// @param geometryInstances The instances to queue for rendering.
    /// @note This function does not invoke any draw calls.
    void setGeometryInstances( const std::vector<std::shared_ptr<GeometryInstance>>& geometryInstances );

    /// Used to remove a geometry instance.
    ///
    /// Once a geometry instance was removed, it will no longer be rendered.
    /// @param geometryInstance The instance to remove.
    void removeGeometryInstance( std::shared_ptr<GeometryInstance> geometryInstance );

    /// Used to remove all geometry instances.
    ///
    /// However, geometries remain loaded and must be deleted explicitely.
    void clearGeometryInstances( );

    /// Used to remove the last geometry instance.
    void popGeometryInstance( );

    /// Used to submit a geometry and set up its buffers.
    ///
    /// Once a geometry was submitted, geometry instances referencing this particular geometry can be drawn.
    /// @param geometry The geometry to submit.
    void submitGeometry( std::shared_ptr<Geometry> geometry );

    /// Used to submit multiple geometries and set up their buffers.
    ///
    /// Once a geometry was submitted, geometry instances referencing this particular geometry can be drawn.
    /// @param geometries The geometries to submit.
    void setGeometries( const std::vector<std::shared_ptr<Geometry>>& geometries );

    /// Used to remove a geometry.
    /// @param geometry The geometry to remove.
    void removeGeometry( std::shared_ptr<Geometry> geometry );

    /// Used to remove a geometry.
    /// @param geometryIndex The geometry's index.
    void removeGeometry( uint32_t geometryIndex );

    /// Used to remove the last geometry and all its instances.
    void popGeometry( );

    /// Used to remove all geometries
    void clearGeometries( );

    /// Used to retrieve a geoemtry based on its path.
    /// @param path The geometry's model's path, relative to the path to assets.
    auto findGeometry( std::string_view path ) const -> std::shared_ptr<Geometry>;

    void setEnvironmentMap( std::string_view path );

    void removeEnvironmentMap( );

    void load( const std::string& path );

    /// Used to set a custom camera.
    /// @param camera A pointer to a RAYEX_NAMESPACE::Camera object.
    void setCamera( std::shared_ptr<Camera> camera );

    void setCamera( int width, int height, const glm::vec3& position = { 0.0F, 0.0F, 3.0F } );

    /// @return Returns a pointer to the renderer's camera.
    auto getCamera( ) const -> std::shared_ptr<Camera> { return _currentCamera; }

  private:
    void initSceneDescriptorSets( );

    void initGeoemtryDescriptorSets( );

    void prepareBuffers( );

    void initCameraBuffer( );

    void uploadCameraBuffer( uint32_t imageIndex );

    void uploadEnvironmentMap( );

    void uploadGeometries( );

    void uploadGeometryInstances( );

    void addDummy( );

    void removeDummy( );

    void translateDummy( );

    void updateSceneDescriptors( );

    void updateGeoemtryDescriptors( );

    void upload( vk::Fence fence, uint32_t imageIndex );

    vkCore::Descriptors _sceneDescriptors;
    vkCore::Descriptors _geometryDescriptors;

    std::vector<vk::DescriptorSet> _sceneDescriptorsets;
    std::vector<vk::DescriptorSet> _geometryDescriptorSets;
    std::vector<vk::DescriptorSet> _textureDescriptorSets;

    vkCore::Cubemap _environmentMap;
    vk::UniqueSampler _immutableSampler;

    std::vector<vkCore::StorageBuffer<uint32_t>> _indexBuffers;
    std::vector<vkCore::StorageBuffer<uint32_t>> _materialIndexBuffers;
    std::vector<vkCore::StorageBuffer<Vertex>> _vertexBuffers;
    vkCore::StorageBuffer<MaterialSSBO> _materialBuffers;
    vkCore::StorageBuffer<GeometryInstanceSSBO> _geometryInstancesBuffer;
    std::vector<std::shared_ptr<vkCore::Texture>> _textures;

    vkCore::UniformBuffer<CameraUbo> _cameraUniformBuffer;

    std::vector<std::shared_ptr<Geometry>> _geometries;
    std::vector<std::shared_ptr<GeometryInstance>> _geometryInstances;

    std::string_view _environmentMapTexturePath;
    bool _useEnvironmentMap    = false;
    bool _removeEnvironmentMap = false;

    bool _uploadGeometryInstancesToBuffer = false;
    bool _uploadEnvironmentMap            = false;
    bool _uploadGeometries                = false;
    bool _dummy                           = false;

    std::unordered_set<std::shared_ptr<Camera>> _cameras; ///< The cameras that can be used for rendering.
    std::shared_ptr<Camera> _currentCamera;               ///< The camera that is currently being used for rendering.

    Settings* _settings = nullptr; ///< Refers to the rendering settings stored in Rayex::settings.
  };
} // namespace RAYEX_NAMESPACE
