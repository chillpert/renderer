#pragma once

#include "pch/stdafx.hpp"

namespace RAYEX_NAMESPACE
{
  /// Exposes all graphic settings supported by the renderer.
  ///
  /// Any necessary pipeline recreations and swapchain recreations will not be performed at the point of calling any setter but instead the next time the renderer
  /// will be updated.
  /// @warning Any function that sets the maximum of a given entity needs to be called before RAYEX_NAMESPACE::Rayex::init().
  /// @ingroup BASE
  /// @todo Add a setUseTotalPathsOnly( bool flag )
  class RX_API Settings
  {
  public:
    friend class Api;
    friend class Rayex;
    friend class Pipeline;
    friend class Scene;

    /// @return Returns the path depth.
    auto getPathDepth( ) const -> uint32_t { return _pathDepth; }

    /// Used to set the path depth.
    ///
    /// The function will trigger a pipeline recreation as soon as possible unless it was explicitely disabled using setAutomaticPipelineRefresh(bool).
    /// If a value higher than the device's maximum supported value is set, it will use the maximum value instead.
    /// @param recursionDepth The new value for the recursion depth.
    void setPathDepth( uint32_t recursionDepth );

    bool getRussianRoulette( ) { return _russianRoulette; }

    void setRussianRoulette( bool flag );

    bool getNextEventEstimation( ) { return _nextEventEstimation; }

    void setNextEventEstimation( bool flag );

    uint32_t getNextEventEstimationMinBounces( ) { return _nextEventEstimationMinBounces; }

    void setNextEventEstimationMinBounces( uint32_t minBounces );

    uint32_t getRussianRouletteMinBounces( ) { return _russianRouletteMinBounces; }

    void setRussianRouletteMinBounces( uint32_t minBounces );

    /// @return Returns the maximum path depth on the GPU.
    auto getMaxPathDepth( ) const -> uint32_t { return _maxPathDepth; }

    /// @return Returns the clear color.
    auto getClearColor( ) const -> const glm::vec4& { return _clearColor; }

    /// Used to changed the clear color.
    ///
    /// The function will trigger a swapchain recreation as soon as possible unless it was explicitely disabled using setAutomaticPipelineRefresh(bool).
    /// @param clearColor The new value for the clear color.
    void setClearColor( const glm::vec4& clearColor );

    /// @return Returns the path to assets.
    auto getAssetsPath( ) const -> std::string_view { return _assetsPath; }

    /// Used to set a path to the directory containing all assets.
    ///
    /// This path should contain all models, textures and shaders.
    /// @param argc The argc parameter that can be retrieved from the main-function's parameters.
    /// @param argv The argv parameter that can be retrieved from the main-function's parameters.
    void setAssetsPath( int argc, char* argv[] );

    /// Used to set a path to the directory containing all assets.
    ///
    /// This path should contain all models, textures and shaders.
    /// @param path The path to assets.
    void setAssetsPath( std::string_view path );

    /// Used to toggle the automatic pipeline recreation.
    /// @param flag If false, the pipelines will not be recreated automatically until this function is called with true.
    void setAutomaticPipelineRefresh( bool flag );

    /// Used to set the maximum amount of geometry (3D models) that can be loaded.
    void setGeometryLimit( size_t amount );

    /// Used to set the maximum amount of geometry instances (instances of 3D models) that can be loaded.
    void setGeometryInstanceLimit( uint32_t amount );

    /// Used to set the maximum amount of textures that can be loaded.
    void setTextureLimit( size_t amount );

    void setPerPixelSampleRate( uint32_t sampleRate );

    auto getPerPixelSampleRate( ) const -> uint32_t { return _perPixelSampleRate; }

    void setAccumulatingFrames( bool flag );

    auto isAccumulatingFrames( ) const -> bool { return _accumulateFrames; }

    void triggerPipelineRefresh( ) { _refreshPipeline = true; }

    void triggerSwapchainRefresh( ) { _refreshSwapchain = true; }

    float getVariance( ) { return _variance; }

    void updateVariance( bool flag );

  private:
    /// This function will be called by Rayex::init() in case the path was not set manually.
    /// @warning This function might file in setting the correct path. That is why it is recommended to set it automatically using setAssetsPath(std::string).
    void setDefaultAssetsPath( );

    bool _refreshPipeline  = false; ///< Keeps track of whether or not the graphics pipeline needs to be recreated.
    bool _refreshSwapchain = false; ///< Keeps track of whether or not the swapchain needs to be recreated.

    size_t _maxGeometryInstances      = 32; ///< Can be set to avoid pipeline recreation everytime a geometry instance is added.
    bool _maxGeometryInstancesChanged = false;
    size_t _maxGeometry               = 16; ///< The maximum amount of geometry (Must be a multiple of minimum storage buffer alignment).
    bool _maxGeometryChanged          = false;
    size_t _maxTextures               = 8; ///< The maximum amount of textures.
    bool _maxTexturesChanged          = false;
    size_t _maxMaterials              = 1024;

    std::string _assetsPath; ///< Where all assets like models, textures and shaders are stored.

    glm::vec4 _clearColor               = glm::vec4( 0.45F, 0.45F, 0.45F, 0.8F ); ///< Stores the clear color.
    uint32_t _maxPathDepth              = 10;                                     ///< The maximum path depth.
    uint32_t _pathDepth                 = 5;                                      ///< The current path depth.
    uint32_t _perPixelSampleRate        = 1;                                      ///< Stores the total amount of samples that will be taken and averaged per pixel.
    uint32_t _russianRouletteMinBounces = 3;

    bool _nextEventEstimation               = false;
    uint32_t _nextEventEstimationMinBounces = 0; // temporary for debugging

    float _variance      = 0.0F;
    bool _updateVariance = false;

    bool _accumulateFrames          = true;
    bool _russianRoulette           = true;
    bool _automaticPipelineRefresh  = false; ///< Keeps track of whether or not the graphics pipelines should be recreated automatically as soon as possible.
    bool _automaticSwapchainRefresh = false; ///< Keeps track of whether or not the swapchain should be recreated automatically as soon as possible.
  };
} // namespace RAYEX_NAMESPACE
