#include "api/Api.hpp"

#include "api/Components.hpp"

#define STB_IMAGE_IMPLEMENTATION

#ifndef VK_KHR_acceleration_structure
  #error The local Vulkan SDK does not support VK_KHR_acceleration_structure. Please update your SDK.
#endif

#define VULKAN_HPP_STORAGE_SHARED
#define VULKAN_HPP_STORAGE_SHARED_EXPORT
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace RAYEX_NAMESPACE
{
  const std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };

  /// @todo Currently always build with debug utils because an error might cause instant
  std::vector<const char*> extensions = { "VK_EXT_debug_utils" };

  //#ifdef RX_DEBUG
  //  std::vector<const char*> extensions = { "VK_EXT_debug_utils" };
  //#else
  //  std::vector<const char*> extensions;
  //#endif

  std::vector<const char*> deviceExtensions = { VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                                VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
                                                VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                                VK_KHR_MAINTENANCE3_EXTENSION_NAME,
                                                VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
                                                VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                                                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                VK_KHR_SHADER_CLOCK_EXTENSION_NAME };

  size_t currentFrame = 0;
  size_t prevFrame    = 0;

  Api::~Api( )
  {
    vkCore::global::device.waitIdle( );

    // Gui needs to be destroyed manually, as RAII destruction will not be possible.
    if ( _gui != nullptr )
    {
      _gui->destroy( );
    }
  }

  void Api::setGui( const std::shared_ptr<Gui>& gui, bool initialize )
  {
    if ( _gui != nullptr )
    {
      recreateSwapchain( );
      _gui->destroy( );
    }

    _gui = gui;

    if ( initialize )
    {
      initGui( );
    }
  }

  void Api::init( )
  {
    RX_LOG_TIME_START( "API start up ..." );

    // Retrieve and add window extensions to other extensions.
    auto windowExtensions = _window->getExtensions( );
    extensions.insert( extensions.end( ), windowExtensions.begin( ), windowExtensions.end( ) );

    // Instance
    _instance = vkCore::initInstanceUnique( layers, extensions, VK_API_VERSION_1_2 );

    // Debug messenger
    _debugMessenger.init( );

    // Surface
    VkSurfaceKHR surface;
    SDL_bool result = SDL_Vulkan_CreateSurface( _window->get( ), static_cast<VkInstance>( vkCore::global::instance ), &surface );

    if ( result != SDL_TRUE )
    {
      RX_ERROR( "Failed to create surface" );
    }

    _surface.init( vk::SurfaceKHR( surface ), _window->getExtent( ) );

    // Physical device
    vkCore::global::physicalDevice = vkCore::initPhysicalDevice( ); // @todo This function does not check if any feature is available when evaluating a device. Additionally, it is pointless to assign vkCore::global::physicalDevice in here because it doesn't need a unique handle.

    // Reassess the support of the preferred surface settings.
    _surface.assessSettings( );

    // Queues
    vkCore::initQueueFamilyIndices( );

    // Logical device
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR asFeatures;
    asFeatures.accelerationStructure = VK_TRUE;

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures;
    rtPipelineFeatures.rayTracingPipeline                  = VK_TRUE;
    rtPipelineFeatures.rayTracingPipelineTraceRaysIndirect = VK_TRUE;
    rtPipelineFeatures.rayTraversalPrimitiveCulling        = VK_TRUE;
    rtPipelineFeatures.pNext                               = &asFeatures;

    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;
    indexingFeatures.runtimeDescriptorArray                        = VK_TRUE;
    indexingFeatures.shaderStorageBufferArrayNonUniformIndexing    = VK_TRUE;
    indexingFeatures.descriptorBindingVariableDescriptorCount      = VK_TRUE;
    indexingFeatures.descriptorBindingPartiallyBound               = VK_TRUE;
    indexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    indexingFeatures.descriptorBindingUpdateUnusedWhilePending     = VK_TRUE;
    indexingFeatures.descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE;
    indexingFeatures.shaderSampledImageArrayNonUniformIndexing     = VK_TRUE;
    indexingFeatures.pNext                                         = &rtPipelineFeatures;

    vk::PhysicalDeviceRobustness2FeaturesEXT robustness2FeaturesEXT;
    robustness2FeaturesEXT.nullDescriptor = VK_TRUE;
    robustness2FeaturesEXT.pNext          = &indexingFeatures;

    vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures;
    rayQueryFeatures.rayQuery = VK_TRUE;
    rayQueryFeatures.pNext    = &robustness2FeaturesEXT;

    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    bufferDeviceAddressFeatures.pNext               = &rayQueryFeatures;

    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.shaderInt64       = VK_TRUE;

    vk::PhysicalDeviceFeatures2 deviceFeatures2 { deviceFeatures };
    deviceFeatures2.pNext = &bufferDeviceAddressFeatures;

    _device = vkCore::initDeviceUnique( deviceExtensions, { }, deviceFeatures2 );

    vkCore::global::device = _device.get( );

    // Retrieve all queue handles.
    vkCore::global::device.getQueue( vkCore::global::graphicsFamilyIndex, 0, &vkCore::global::graphicsQueue );
    vkCore::global::device.getQueue( vkCore::global::transferFamilyIndex, 0, &vkCore::global::transferQueue );

    // Command pools
    _graphicsCmdPool                = vkCore::initCommandPoolUnique( vkCore::global::graphicsFamilyIndex, vk::CommandPoolCreateFlagBits::eResetCommandBuffer );
    vkCore::global::graphicsCmdPool = _graphicsCmdPool.get( );

    _transferCmdPool                = vkCore::initCommandPoolUnique( vkCore::global::transferFamilyIndex, { } );
    vkCore::global::transferCmdPool = _transferCmdPool.get( );

    // Post processing renderer
    //_postProcessingRenderer.initDepthImage( _surface.getExtent( ) );
    _postProcessingRenderer.initRenderPass( _surface.getFormat( ) );

    // Swapchain
    _swapchain.init( &_surface, _postProcessingRenderer.getRenderPass( ).get( ) );
    _settings._refreshSwapchain = false;

    // GUI
    initGui( );

    // Create fences and semaphores.
    _sync.init( );

    // Path tracer
    _pathTracer.init( );
    _settings._maxPathDepth = _pathTracer.getCapabilities( ).pipelineProperties.maxRayRecursionDepth;
    _pathTracer.initVarianceBuffer( static_cast<float>( _window->getWidth( ) ), static_cast<float>( _window->getHeight( ) ) );

    _scene.prepareBuffers( );

    // Descriptor sets and layouts
    _pathTracer.initDescriptorSet( );
    _scene.initSceneDescriptorSets( );
    _scene.initGeoemtryDescriptorSets( );

    // Default environment map to assure start up.
    _scene.setEnvironmentMap( "" );
    _scene.uploadEnvironmentMap( );
    _scene.removeEnvironmentMap( );

    // Update scene descriptor sets.
    _scene.updateSceneDescriptors( );

    // Initialize the path tracing pipeline.
    initPipelines( );

    _pathTracer.createStorageImage( _swapchain.getExtent( ) );
    _pathTracer.createShaderBindingTable( );

    // Post processing renderer
    _postProcessingRenderer.initDescriptorSet( );
    _postProcessingRenderer.initPipeline( );
    _postProcessingRenderer.updateDescriptors( _pathTracer.getStorageImageInfo( ) );

    // Initialize and record swapchain command buffers.
    _swapchainCommandBuffers.init( _graphicsCmdPool.get( ), vkCore::global::swapchainImageCount, vk::CommandBufferUsageFlagBits::eRenderPassContinue );

    RX_LOG_TIME_STOP( "API finished" );
  }

  void Api::update( )
  {
    updateSettings( );

#ifdef RX_VARIANCE_ESTIMATOR
    // update variance
    if ( _settings._updateVariance )
    {
      static uint32_t counter = 0;
      const int maxSize       = 100;
      static std::vector<std::array<float, maxSize>> ppVariances; // vector of vector. outer for each pixel and inner for each accumulated sample

      auto extent     = _swapchain.getExtent( );
      auto pixelCount = 1;
      //extent.width* extent.height;
      ppVariances.resize( pixelCount ); // as many ppVariances as pixels

      RX_ASSERT( maxSize > _settings._perPixelSampleRate, "Variance Estimates Out Of Bound" );

      // 1. Gathering stage
      bool finishedGatheringPp = false;
      if ( counter < _settings._perPixelSampleRate )
      {
        // For each new sample of a frame for each pixel set estimated variance
        for ( uint32_t i = 0; i < pixelCount; ++i )
        {
          ppVariances[i][counter] = _pathTracer.getPixelVariance( i );
          std::cout << ppVariances[i][counter] << std::endl;
        }

        ++counter;
      }
      else
      {
        finishedGatheringPp = true;
      }

      // 2. Averaging State
      if ( finishedGatheringPp )
      {
        counter = 0;

        std::vector<float> ppSum( pixelCount, 0.0F );

        // Sum up samples for each pixel and store results separately
        for ( uint32_t i = 0; i < _settings._perPixelSampleRate; ++i )
        {
          for ( uint32_t j = 0; j < pixelCount; ++j )
          {
            ppSum[j] += ppVariances[j][i];
          }
        }

        float avg = 0.0F;
        // calculate final average
        for ( size_t i = 0; i < ppSum.size( ); ++i )
        {
          // Do not forget to take the average of the previous sample sum per pixel
          avg += ppSum[i] / static_cast<float>( _settings._perPixelSampleRate );
        }

        _settings._variance = avg / pixelCount;
      }
    }

    //std::cout << _pathTracer.getPixelVariance( ) << std::endl;
#endif

    uint32_t imageIndex        = _swapchain.getCurrentImageIndex( );
    uint32_t maxFramesInFlight = static_cast<uint32_t>( _sync.getMaxFramesInFlight( ) );

    _scene.uploadCameraBuffer( imageIndex % maxFramesInFlight );

    // If the scene is empty add a dummy triangle so that the acceleration structures can be built successfully.

    // Move dummy behind camera
    if ( _scene._dummy )
    {
      _scene.translateDummy( );
    }

    if ( _scene._geometryInstances.empty( ) )
    {
      _scene.addDummy( );
    }
    else if ( _scene._dummy )
    {
      _scene.removeDummy( );
    }

    if ( _scene._uploadEnvironmentMap )
    {
      _sync.waitForFrame( prevFrame );
      _scene.uploadEnvironmentMap( );
      _scene.updateSceneDescriptors( );
    }

    if ( _scene._uploadGeometries )
    {
      _scene.uploadGeometries( );
      _scene.updateGeoemtryDescriptors( );
    }

    if ( _scene._uploadGeometryInstancesToBuffer )
    {
      _scene.uploadGeometryInstances( );

      // @TODO Try to call this as few times as possible.
      _pathTracer.createBottomLevelAS( _scene._vertexBuffers, _scene._indexBuffers, _scene._geometries );
      _pathTracer.buildTlas( _scene._geometryInstances, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate );
      _pathTracer.updateDescriptors( );
    }
    else
    {
      _pathTracer.updateTlas( _scene._geometryInstances, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate );
    }

    // Increment frame counter for jitter cam.
    if ( _settings._accumulateFrames )
    {
      ++components::frameCount;
    }
    else
    {
      components::frameCount = -1;
    }
  }

  void Api::prepareFrame( )
  {
    _swapchain.acquireNextImage( _sync.getImageAvailableSemaphore( currentFrame ), nullptr );
  }

  void Api::submitFrame( )
  {
    uint32_t imageIndex = _swapchain.getCurrentImageIndex( );
    size_t imageIndex_t = static_cast<size_t>( imageIndex );

    // Check if a previous frame is using the current image.
    if ( _sync.getImageInFlight( imageIndex ) )
    {
      _sync.waitForFrame( currentFrame );
    }

    // This will mark the current image to be in use by this frame.
    _sync.getImageInFlight( imageIndex_t ) = _sync.getInFlightFence( currentFrame );

    vk::CommandBuffer cmdBuf = _swapchainCommandBuffers.get( imageIndex );

    // Reset the signaled state of the current frame's fence to the unsignaled one.
    auto currentInFlightFence_t = _sync.getInFlightFence( currentFrame );
    vkCore::global::device.resetFences( 1, &currentInFlightFence_t );

    // Submits / executes the current image's / framebuffer's command buffer.
    vk::PipelineStageFlags pWaitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    auto waitSemaphore    = _sync.getImageAvailableSemaphore( currentFrame );
    auto signaleSemaphore = _sync.getFinishedRenderSemaphore( currentFrame );

    vk::SubmitInfo submitInfo( 1,                   // waitSemaphoreCount
                               &waitSemaphore,      // pWaitSemaphores
                               &pWaitDstStageMask,  // pWaitDstStageMask
                               1,                   // commandBufferCount
                               &cmdBuf,             // pCommandBuffers
                               1,                   // signalSemaphoreCount
                               &signaleSemaphore ); // pSignalSemaphores

    vkCore::global::graphicsQueue.submit( submitInfo, currentInFlightFence_t );

    // Tell the presentation engine that the current image is ready.
    vk::PresentInfoKHR presentInfo( 1,                          // waitSemaphoreCount
                                    &signaleSemaphore,          // pWaitSemaphores
                                    1,                          // swapchainCount
                                    &vkCore::global::swapchain, // pSwapchains
                                    &imageIndex,                // pImageIndices
                                    nullptr );                  // pResults

    // This try catch block is only necessary on Linux for whatever reason. Without it, resizing the window will result in an unhandled throw of vk::Result::eErrorOutOfDateKHR.
    try
    {
      vk::Result result = vkCore::global::graphicsQueue.presentKHR( presentInfo );
      if ( result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR )
      {
        _settings.triggerSwapchainRefresh( );
        RX_WARN( "Swapchain out of data or suboptimal." );
      }
    }
    catch ( ... )
    {
      _settings.triggerSwapchainRefresh( );
    }

    prevFrame    = currentFrame;
    currentFrame = ( currentFrame + 1 ) % _sync.getMaxFramesInFlight( );
  }

  void Api::updateSettings( )
  {
    if ( _settings._maxGeometryChanged || _settings._maxTexturesChanged )
    {
      _sync.waitForFrame( prevFrame );

      _settings._maxGeometryChanged = false;
      _settings._maxTexturesChanged = false;

      _scene._vertexBuffers.resize( _settings._maxGeometry );
      _scene._indexBuffers.resize( _settings._maxGeometry );
      _scene._materialIndexBuffers.resize( _settings._maxGeometry );
      _scene._textures.resize( _settings._maxTextures );

      _scene.initGeoemtryDescriptorSets( );

      _settings._refreshPipeline = true;
    }

    // Handle pipeline refresh
    if ( _settings._refreshPipeline )
    {
      _settings._refreshPipeline = false;

      // Calling wait idle, because pipeline recreation is assumed to be a very rare event to happen.
      vkCore::global::device.waitIdle( );

#ifdef RX_COPY_ASSETS
      // Copies shader resources to binary output directory. This way a shader can be changed during runtime.
      // Make sure only to edit the ones in /assets/shaders and not in /build/bin/debug/assets/shaders as the latter gets overridden.
      RX_INFO( "Copying shader resources to binary output directory.\nFrom: ", RX_ASSETS_PATH, "shaders\nto: ", RX_PATH_TO_LIBRARY, "shaders" );
      std::filesystem::copy( RX_ASSETS_PATH "shaders", RX_PATH_TO_LIBRARY "shaders", std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive );

      //std::filesystem::copy( RX_ASSETS_PATH "shaders", RX_PATH_TO_LIBRARY "shaders", std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive );
      //std::filesystem::copy( RX_ASSETS_PATH "models", RX_PATH_TO_LIBRARY "models", std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive );
      //std::filesystem::copy( RX_ASSETS_PATH "DroidSans.ttf", RX_PATH_TO_LIBRARY "DroidSans.ttf", std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive );

#endif

      initPipelines( );
      _pathTracer.createShaderBindingTable( );
    }

    // Handle swapchain refresh
    if ( _settings._refreshSwapchain )
    {
      _settings._refreshSwapchain = false;

      recreateSwapchain( );
    }
  }

  void Api::render( )
  {
    update( );

    // If the window is minimized then simply do not render anything anymore.
    if ( _window->minimized( ) )
    {
      return;
    }

    // If the window size has changed the swapchain has to be recreated.
    if ( _window->changed( ) )
    {
      _camera->_updateProj = true;
      return;
    }

    prepareFrame( );

    recordSwapchainCommandBuffers( );

    submitFrame( );
  }

  void Api::recreateSwapchain( )
  {
    RX_LOG_TIME_START( "Re-creating swapchain ..." );

    // Waiting idle because this event is considered to be very rare.
    vkCore::global::device.waitIdle( );

    // Clean up existing swapchain and dependencies.
    _swapchain.destroy( );

    // Recreating the swapchain.
    _swapchain.init( &_surface, _postProcessingRenderer.getRenderPass( ).get( ) );

    // Recreate storage image with the new swapchain image size and update the path tracing descriptor set to use the new storage image view.
    _pathTracer.createStorageImage( _swapchain.getExtent( ) );

    const auto& storageImageInfo = _pathTracer.getStorageImageInfo( );
    _postProcessingRenderer.updateDescriptors( storageImageInfo );

    _pathTracer.updateDescriptors( );

    if ( _gui != nullptr )
    {
      _gui->recreate( _swapchain.getExtent( ) );
    }

    // Update the camera screen size to avoid image stretching.
    auto screenSize = _swapchain.getExtent( );
    _camera->setSize( screenSize.width, screenSize.height );

    _settings._refreshSwapchain = false;

    RX_LOG_TIME_STOP( "Finished re-creating swapchain" );
  }

  void Api::initPipelines( )
  {
    RX_LOG_TIME_START( "Initializing graphic pipelines ..." );

    // path tracing pipeline
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = { _pathTracer.getDescriptorSetLayout( ),
                                                                  _scene._sceneDescriptors.layout.get( ),
                                                                  _scene._geometryDescriptors.layout.get( ) };

    _pathTracer.createPipeline( descriptorSetLayouts );
    _settings._refreshPipeline = false;

    RX_LOG_TIME_STOP( "Finished graphic pipelines initialization" );
  }

  void Api::initGui( )
  {
    if ( _gui != nullptr )
    {
      _gui->init( _window->get( ), &_surface, _swapchain.getExtent( ), _postProcessingRenderer.getRenderPass( ).get( ) );
    }
  }

  void Api::recordSwapchainCommandBuffers( )
  {
    RX_ASSERT( _pathTracer.getPipeline( ) && _postProcessingRenderer.getPipeline( ), "Can not record swapchain command buffers because the pipelines have not been initialized yet." );

    // Wait for previous frame to finish command buffer execution.
    _sync.waitForFrame( prevFrame );

    PtPushConstants chitPc = { _settings._clearColor,
                               components::frameCount,
                               _settings._perPixelSampleRate,
                               _settings._pathDepth,
                               static_cast<uint32_t>( _scene._useEnvironmentMap ),
                               static_cast<uint32_t>( _settings._russianRoulette ),
                               _settings._russianRouletteMinBounces,
                               _settings._nextEventEstimation,
                               _settings._nextEventEstimationMinBounces };

    // Start recording the swapchain framebuffers?
    for ( size_t imageIndex = 0; imageIndex < _swapchainCommandBuffers.get( ).size( ); ++imageIndex )
    {
      vk::CommandBuffer cmdBuf = _swapchainCommandBuffers.get( imageIndex );

      _swapchainCommandBuffers.begin( imageIndex );
      {
        cmdBuf.pushConstants( _pathTracer.getPipelineLayout( ),                                                                                  // layout
                              vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eClosestHitKHR, // stageFlags
                              0,                                                                                                                 // offset
                              sizeof( PtPushConstants ),                                                                                         // size
                              &chitPc );                                                                                                         // pValues

        cmdBuf.bindPipeline( vk::PipelineBindPoint::eRayTracingKHR, _pathTracer.getPipeline( ) );

        size_t index = imageIndex % _sync.getMaxFramesInFlight( );

        std::vector<vk::DescriptorSet> descriptorSets = { _pathTracer.getDescriptorSet( index ),
                                                          _scene._sceneDescriptorsets[index],
                                                          _scene._geometryDescriptorSets[index] };

        cmdBuf.bindDescriptorSets( vk::PipelineBindPoint::eRayTracingKHR,           // pipelineBindPoint
                                   _pathTracer.getPipelineLayout( ),                // layout
                                   0,                                               // first set
                                   static_cast<uint32_t>( descriptorSets.size( ) ), // descriptor set count
                                   descriptorSets.data( ),                          // descriptor sets
                                   0,                                               // dynamic offset count
                                   nullptr );                                       // dynamic offsets

        // First pass (path tracing)
        _pathTracer.pathTrace( cmdBuf, _swapchain.getImage( imageIndex ), _swapchain.getExtent( ) );

        // Second pass
        _postProcessingRenderer.beginRenderPass( cmdBuf, _swapchain.getFramebuffer( imageIndex ), _swapchain.getExtent( ) );
        {
          // 2. Post processing
          _postProcessingRenderer.render( cmdBuf, _swapchain.getExtent( ), index );

          // 3. ImGui
          if ( _gui != nullptr )
          {
            _gui->renderDrawData( cmdBuf );
          }
        }
        _postProcessingRenderer.endRenderPass( cmdBuf );
      }
      _swapchainCommandBuffers.end( imageIndex );
    }
  }
} // namespace RAYEX_NAMESPACE
