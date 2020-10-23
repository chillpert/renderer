#include "api/Api.hpp"

#include "api/utility/Helpers.hpp"
#include "api/utility/Initializers.hpp"
#include "api/utility/Util.hpp"

namespace RAYEX_NAMESPACE
{
  const std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };
#ifdef RX_DEBUG
  std::vector<const char*> extensions = { "VK_KHR_get_physical_device_properties2", "VK_EXT_debug_utils" };
#else
  std::vector<const char*> extensions = { "VK_KHR_get_physical_device_properties2" };
#endif

  std::vector<const char*> deviceExtensions = {
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
    VK_KHR_MAINTENANCE3_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  size_t currentFrame = 0;
  size_t prevFrame    = 0;

  std::vector<GeometryInstanceSSBO> memAlignedGeometryInstances;
  std::vector<MeshSSBO> memAlignedMeshes;
  std::vector<DirectionalLightSSBO> memAlignedDirectionalLights;
  std::vector<PointLightSSBO> memAlignedPointLights;

  CameraUbo cameraUbo;

  vk::Viewport viewport; ///< The application's viewport.
  vk::Rect2D scissor;    ///< The application's scissor.

  // Defines the maximum amount of frames that will be processed concurrently.
  const size_t maxFramesInFlight = 2;

  Api::Api( std::shared_ptr<Window> window, std::shared_ptr<Camera> camera ) :
    _window( std::move( window ) ),
    _camera( std::move( camera ) ),
    _gui( nullptr )
  {
  }

  Api::~Api( )
  {
    components::device.waitIdle( );

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

  void Api::initBase( )
  {
    RX_LOG_TIME_START( "Initializing Vulkan (base) ..." );

    // Retrieve and add window extensions to other extensions.
    auto windowExtensions = _window->getExtensions( );
    extensions.insert( extensions.end( ), windowExtensions.begin( ), windowExtensions.end( ) );

    // Instance
    _instance = vk::Initializer::initInstance( layers, extensions );

    // Debug messenger
    _debugMessenger.init( );

    // Surface
    _surface.init( _window );

    // Physical device
    components::physicalDevice = vk::Initializer::initPhysicalDevice( );

    // Reassess the support of the preferred surface settings.
    _surface.assessSettings( );

    // Queues
    vk::Initializer::initQueueFamilyIndices( );

    // Logical device
    _device = vk::Initializer::initDevice( deviceExtensions );

    // Retrieve all queue handles.
    components::device.getQueue( components::graphicsFamilyIndex, 0, &components::graphicsQueue );
    components::device.getQueue( components::transferFamilyIndex, 0, &components::transferQueue );

    // Render pass
    initRenderPass( );

    // Swapchain
    _swapchain.init( &_surface, _renderPass.get( ) );
    _settings->_refreshSwapchain = false;

    // Command pools
    _graphicsCmdPool            = vk::Initializer::initCommandPoolUnique( components::graphicsFamilyIndex, vk::CommandPoolCreateFlagBits::eResetCommandBuffer );
    components::graphicsCmdPool = _graphicsCmdPool.get( );

    _transferCmdPool            = vk::Initializer::initCommandPoolUnique( components::transferFamilyIndex, { } );
    components::transferCmdPool = _transferCmdPool.get( );

    // GUI
    initGui( );

    // Create fences and semaphores.
    initSyncObjects( );

    // Resize and initialize buffers with "dummy data".
    // The advantage of doing this is that the buffers are all initialized right away (even though it is invalid data) and
    // this makes it possible to call fill instead of init again, when changing any of the data below.
    std::vector<GeometryInstance> geometryInstances( _settings->_maxGeometryInstances );
    _geometryInstancesBuffer.init<GeometryInstance>( geometryInstances );

    std::vector<DirectionalLightSSBO> directionalLights( _settings->_maxDirectionalLights );
    _directionalLightsBuffer.init<DirectionalLightSSBO>( directionalLights );

    std::vector<PointLightSSBO> pointLights( _settings->_maxPointLights );
    _pointLightsBuffer.init<PointLightSSBO>( pointLights );

    _vertexBuffers.resize( _settings->_maxGeometry );
    _indexBuffers.resize( _settings->_maxGeometry );
    _meshBuffers.resize( _settings->_maxGeometry );

    RX_LOG_TIME_STOP( "Finished initializing Vulkan (base)" );
  }

  void Api::initScene( )
  {
    RX_LOG_TIME_START( "Initializing Vulkan (scene) ..." );

    // Uniform buffers for camera
    _cameraUniformBuffer.init<CameraUbo>( );

    // Descriptor sets and layouts
    initDescriptorSets( );

    // Update RT scene descriptor sets.
    updateSceneDescriptors( );

    // Update the vertex and index SSBO descriptors for the ray tracing shaders.
    updateRayTracingModelData( );

    // Initialize a rasterization and raytracing pipeline.
    initPipelines( );

    // Ray tracing
    _rtBuilder.init( );
    _rtBuilder.createStorageImage( _swapchain.getExtent( ) );
    _rtBuilder.createShaderBindingTable( );

    _settings->_maxRecursionDepth = _rtBuilder.getRtProperties( ).maxRecursionDepth;

    // Init and record swapchain command buffers.
    _swapchainCommandBuffers.init( _graphicsCmdPool.get( ), components::swapchainImageCount, vk::CommandBufferUsageFlagBits::eRenderPassContinue );

    RX_LOG_TIME_STOP( "Finished initializing Vulkan (scene)" );
  }

  void Api::update( )
  {
    updateSettings( );

    updateUniformBuffers( );

    // If the scene is empty add a dummy triangle so that a TLAS can be built successfully.
    if ( _scene->_geometryInstances.empty( ) )
    {
      auto triangle = std::make_shared<Geometry>( );
      Vertex v1;
      v1.normal = glm::vec3( 0.0F, 1.0F, 0.0F );
      v1.pos    = glm::vec3( -0.00001F, 0.0F, 0.00001F );

      Vertex v2;
      v2.normal = glm::vec3( 0.0F, 1.0F, 0.0F );
      v2.pos    = glm::vec3( 0.00001F, 0.0F, 0.00001F );

      Vertex v3;
      v3.normal = glm::vec3( 0.0F, 1.0F, 0.0F );
      v3.pos    = glm::vec3( 0.00001F, 0.0F, -0.00001F );

      triangle->vertices      = { v1, v2, v3 };
      triangle->indices       = { 0, 1, 2 };
      triangle->geometryIndex = components::geometryIndex++;
      triangle->meshes.push_back( { { }, 0 } );

      _scene->submitGeometry( triangle );
      _scene->submitGeometryInstance( instance( triangle ) );
    }

    // Init geometry storage buffers.
    if ( _scene->_uploadGeometries )
    {
      _scene->_uploadGeometries = false;

      for ( size_t i = 0; i < _scene->_geometries.size( ); ++i )
      {
        if ( i < _scene->_geometries.size( ) )
        {
          if ( _scene->_geometries[i] != nullptr )
          {
            if ( !_scene->_geometries[i]->initialized )
            {
              _vertexBuffers[i].init( _scene->_geometries[i]->vertices );
              _indexBuffers[i].init( _scene->_geometries[i]->indices );

              memAlignedMeshes.resize( _scene->_geometries[i]->meshes.size( ) );
              std::transform( _scene->_geometries[i]->meshes.begin( ),
                              _scene->_geometries[i]->meshes.end( ),
                              memAlignedMeshes.begin( ),
                              []( const Mesh& mesh ) { return MeshSSBO { glm::vec4( mesh.material.ambient, -1.0F ),
                                                                         glm::vec4( mesh.material.diffuse, -1.0F ),
                                                                         glm::vec4( mesh.material.specular, -1.0F ),
                                                                         { },
                                                                         { },
                                                                         { },
                                                                         mesh.indexOffset }; } );
              _meshBuffers[i].init<MeshSSBO>( memAlignedMeshes );

              _scene->_geometries[i]->initialized = true;
            }
          }
        }
      }

      updateRayTracingModelData( ); // Contains descriptors for vertices and indices.
    }

    if ( _scene->_uploadGeometryInstancesToBuffer )
    {
      _scene->_uploadGeometryInstancesToBuffer = false;

      if ( !_scene->_geometryInstances.empty( ) )
      {
        // Dereference pointers and store values in new vector that will be uploaded.
        memAlignedGeometryInstances.resize( _scene->_geometryInstances.size( ) );
        std::transform( _scene->_geometryInstances.begin( ),
                        _scene->_geometryInstances.end( ),
                        memAlignedGeometryInstances.begin( ),
                        []( std::shared_ptr<GeometryInstance> instance ) { return GeometryInstanceSSBO { instance->transform,
                                                                                                         instance->transformIT,
                                                                                                         instance->geometryIndex }; } );

        _geometryInstancesBuffer.upload<GeometryInstanceSSBO>( memAlignedGeometryInstances );
        updateAccelerationStructures( );
      }
    }

    if ( _scene->_uploadDirectionalLightsToBuffer )
    {
      _scene->_uploadDirectionalLightsToBuffer = false;

      if ( !_scene->_directionalLights.empty( ) )
      {
        memAlignedDirectionalLights.resize( _scene->_directionalLights.size( ) );
        std::transform( _scene->_directionalLights.begin( ),
                        _scene->_directionalLights.end( ),
                        memAlignedDirectionalLights.begin( ),
                        []( std::shared_ptr<DirectionalLight> light ) { return DirectionalLightSSBO { glm::vec4( light->ambient, light->ambientIntensity ),
                                                                                                      glm::vec4( light->diffuse, light->diffuseIntensity ),
                                                                                                      glm::vec4( light->specular, light->specularIntensity ),
                                                                                                      glm::vec4( light->direction, 1.0F ) }; } );

        _directionalLightsBuffer.upload<DirectionalLightSSBO>( memAlignedDirectionalLights );
      }
    }

    if ( _scene->_uploadPointLightsToBuffer )
    {
      _scene->_uploadPointLightsToBuffer = false;

      if ( !_scene->_pointLights.empty( ) )
      {
        memAlignedPointLights.resize( _scene->_pointLights.size( ) );
        std::transform( _scene->_pointLights.begin( ),
                        _scene->_pointLights.end( ),
                        memAlignedPointLights.begin( ),
                        []( std::shared_ptr<PointLight> light ) { return PointLightSSBO { glm::vec4( light->ambient, light->ambientIntensity ),
                                                                                          glm::vec4( light->diffuse, light->diffuseIntensity ),
                                                                                          glm::vec4( light->specular, light->specularIntensity ),
                                                                                          glm::vec4( light->position, 1.0F ) }; } );

        _pointLightsBuffer.upload<PointLightSSBO>( memAlignedPointLights );
      }
    }

    if ( _settings->getJitterCamEnabled( ) )
    {
      if ( components::frameCount >= _settings->_jitterCamSampleRate )
      {
        return;
      }

      // Increment frame counter for jitter cam.
      ++components::frameCount;
    }
  }

  auto Api::prepareFrame( ) -> bool
  {
    // If the window is minimized then simply do not render anything anymore.
    if ( _window->minimized( ) )
    {
      return true;
    }

    // If the window size has changed the swapchain has to be recreated.
    if ( _window->changed( ) || _needSwapchainRecreate )
    {
      _camera->_updateProj = true;

      _needSwapchainRecreate = false;
      recreateSwapchain( );
      return true;
    }

    _swapchain.acquireNextImage( _imageAvailableSemaphores[currentFrame].get( ), nullptr );

    // Wait for the current frame's fences.
    vk::Result result = components::device.waitForFences( 1, &_inFlightFences[currentFrame].get( ), VK_TRUE, UINT64_MAX );
    RX_ASSERT( result == vk::Result::eSuccess, "Failed to wait for fences." );

    return false;
  }

  auto Api::submitFrame( ) -> bool
  {
    uint32_t imageIndex = _swapchain.getCurrentImageIndex( );

    // Check if a previous frame is using the current image.
    if ( _imagesInFlight[imageIndex] )
    {
      vk::Result result = components::device.waitForFences( 1, &_imagesInFlight[currentFrame], VK_TRUE, UINT64_MAX );
      RX_ASSERT( result == vk::Result::eSuccess, "Failed to wait for fences." );
    }

    // This will mark the current image to be in use by this frame.
    _imagesInFlight[imageIndex] = _inFlightFences[currentFrame].get( );

    // Add GUI command buffer to swapchain command buffer if GUI is enabled.
    std::vector<vk::CommandBuffer> commandBuffers = { _swapchainCommandBuffers.get( )[imageIndex] };
    if ( _gui != nullptr )
    {
      commandBuffers.push_back( _gui->getCommandBuffer( imageIndex ) );
    }

    // Reset the signaled state of the current frame's fence to the unsignaled one.
    components::device.resetFences( 1, &_inFlightFences[currentFrame].get( ) );

    // Submits / executes the current image's / framebuffer's command buffer.
    auto pWaitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto submitInfo        = vk::Helper::getSubmitInfo( _imageAvailableSemaphores[currentFrame].get( ), _finishedRenderSemaphores[currentFrame].get( ), commandBuffers, pWaitDstStageMask );
    components::graphicsQueue.submit( submitInfo, _inFlightFences[currentFrame].get( ) );

    // Tell the presentation engine that the current image is ready.
    auto presentInfo = vk::Helper::getPresentInfoKHR( _finishedRenderSemaphores[currentFrame].get( ), imageIndex );

    try
    {
      vk::Result result = components::graphicsQueue.presentKHR( presentInfo );
      if ( result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR )
      {
        _needSwapchainRecreate = true;
        RX_WARN( "Swapchain out of data or suboptimal." );
      }
    }
    catch ( ... )
    {
      _needSwapchainRecreate = true;
    }

    prevFrame    = currentFrame;
    currentFrame = ( currentFrame + 1 ) % maxFramesInFlight;
    return false;
  }

  auto Api::render( ) -> bool
  {
    if ( _gui != nullptr )
    {
      _gui->newFrame( );
      _gui->render( );
      _gui->endRender( );
    }

    if ( prepareFrame( ) )
    {
      return true;
    }

    // Wait for previous frame to finish command buffer execution.
    vk::Result result = components::device.waitForFences( 1, &_inFlightFences[prevFrame].get( ), VK_TRUE, UINT64_MAX );
    RX_ASSERT( result == vk::Result::eSuccess, "Failed to wait for fences." );
    recordSwapchainCommandBuffers( );

    if ( _gui != nullptr )
    {
      _gui->renderDrawData( _swapchain.getCurrentImageIndex( ) );
    }

    if ( submitFrame( ) )
    {
      return true;
    }

    return true;
  }

  void Api::recreateSwapchain( )
  {
    RX_LOG_TIME_START( "Re-creating swapchain ..." );

    components::device.waitIdle( );

    // Clean up existing swapchain and dependencies.
    _swapchainCommandBuffers.free( );
    _swapchain.destroy( );

    // Recreating the swapchain.
    _swapchain.init( &_surface, _renderPass.get( ) );

    // Recreate storage image with the new swapchain image size and update the ray tracing descriptor set to use the new storage image view.
    _rtBuilder.createStorageImage( _swapchain.getExtent( ) );

    vk::WriteDescriptorSetAccelerationStructureKHR tlasInfo( 1,
                                                             &_rtBuilder.getTlas( ).as.as );

    vk::DescriptorImageInfo storageImageInfo( nullptr,
                                              _rtBuilder.getStorageImageView( ),
                                              vk::ImageLayout::eGeneral );

    _rtDescriptors.bindings.write( _rtDescriptorSets, 0, &tlasInfo );
    _rtDescriptors.bindings.write( _rtDescriptorSets, 1, &storageImageInfo );
    _rtDescriptors.bindings.update( );

    // Swapchain command buffers
    _swapchainCommandBuffers.init( _graphicsCmdPool.get( ), components::swapchainImageCount, vk::CommandBufferUsageFlagBits::eRenderPassContinue );
    recordSwapchainCommandBuffers( );

    if ( _gui != nullptr )
    {
      _gui->recreate( _swapchain.getExtent( ), _swapchain.getImageViews( ) );
    }

    // Update the camera screen size to avoid image stretching.
    auto screenSize = _swapchain.getExtent( );
    _camera->setSize( screenSize.width, screenSize.height );

    _settings->_refreshSwapchain = false;

    RX_LOG_TIME_STOP( "Finished re-creating swapchain" );
  }

  void Api::updateAccelerationStructures( )
  {
    RX_LOG_TIME_START( "Updating acceleration structures ..." );

    // @TODO Try to call this as few times as possible.
    _rtBuilder.createBottomLevelAS( _vertexBuffers, _indexBuffers );
    _rtBuilder.createTopLevelAS( _scene->_geometryInstances );

    // Update ray tracing descriptor set.
    vk::WriteDescriptorSetAccelerationStructureKHR tlasInfo( 1,
                                                             &_rtBuilder.getTlas( ).as.as );

    vk::DescriptorImageInfo storageImageInfo( nullptr,
                                              _rtBuilder.getStorageImageView( ),
                                              vk::ImageLayout::eGeneral );

    _rtDescriptors.bindings.write( _rtDescriptorSets, 0, &tlasInfo );
    _rtDescriptors.bindings.write( _rtDescriptorSets, 1, &storageImageInfo );
    _rtDescriptors.bindings.update( );

    RX_LOG_TIME_STOP( "Finished updating acceleration structures" );
  }

  void Api::initPipelines( )
  {
    RX_LOG_TIME_START( "Initializing graphic pipelines ..." );

    // Ray tracing pipeline
    std::vector<vk::DescriptorSetLayout> allRtDescriptorSetLayouts = { _rtDescriptors.layout.get( ),
                                                                       _rtSceneDescriptors.layout.get( ),
                                                                       _geometryDescriptors.layout.get( ) };
    //_indexDataDescriptors.layout.get( ) };
    _rtBuilder.createPipeline( allRtDescriptorSetLayouts, _settings );

    // Rasterization pipeline
    glm::vec2 extent = { static_cast<float>( _swapchain.getExtent( ).width ), static_cast<float>( _swapchain.getExtent( ).height ) };
    viewport         = vk::Viewport( 0.0F, 0.0F, extent.x, extent.y, 0.0F, 1.0F );
    scissor          = vk::Rect2D( 0, _swapchain.getExtent( ) );

    std::vector<vk::DescriptorSetLayout> allRsDescriptorSetLayouts = { _rsSceneDescriptors.layout.get( ) };

    _rsPipeline.init( allRsDescriptorSetLayouts, _renderPass.get( ), viewport, scissor, _settings );

    _pipelinesReady             = true;
    _settings->_refreshPipeline = false;

    RX_LOG_TIME_STOP( "Finished graphic pipelines initialization" );
  }

  void Api::initRenderPass( )
  {
    auto colorAttachmentDescription = vk::Helper::getAttachmentDescription( _surface.getFormat( ) );

    vk::AttachmentReference colorAttachmentReference( 0,                                          // attachment
                                                      vk::ImageLayout::eColorAttachmentOptimal ); // layout

    auto depthAttachmentDescription = vk::Helper::getDepthAttachmentDescription( getSupportedDepthFormat( components::physicalDevice ) );

    vk::AttachmentReference depthAttachmentRef( 1,                                                 // attachment
                                                vk::ImageLayout::eDepthStencilAttachmentOptimal ); // layout

    vk::SubpassDescription subpassDescription( { },                              // flags
                                               vk::PipelineBindPoint::eGraphics, // pipelineBindPoint
                                               0,                                // inputAttachmentsCount
                                               nullptr,                          // pInputAttachments
                                               1,                                // colorAttachmentsCount
                                               &colorAttachmentReference,        // pColorAttachments
                                               nullptr,                          // pResolveAttachments
                                               &depthAttachmentRef,              // pDepthStencilAttachment
                                               0,                                // preserveAttachemntCount
                                               nullptr );                        // pPreserveAttachments

    std::vector<vk::SubpassDependency> subpassDependencies( 2 );

    subpassDependencies[0] = { VK_SUBPASS_EXTERNAL,                                                                  // srcSubpass
                               0,                                                                                    // dstSubpass
                               vk::PipelineStageFlagBits::eBottomOfPipe,                                             // srcStageMask
                               vk::PipelineStageFlagBits::eColorAttachmentOutput,                                    // dstStageMask
                               vk::AccessFlagBits::eMemoryRead,                                                      // srcAccessMask
                               vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, // dstAccessMask
                               vk::DependencyFlagBits::eByRegion };                                                  // dependencyFlags

    subpassDependencies[1] = { 0,                                                                                    // srcSubpass
                               VK_SUBPASS_EXTERNAL,                                                                  // dstSubpass
                               vk::PipelineStageFlagBits::eColorAttachmentOutput,                                    // srcStageMask
                               vk::PipelineStageFlagBits::eBottomOfPipe,                                             // dstStageMask
                               vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, // srcAccessMask
                               vk::AccessFlagBits::eMemoryRead,                                                      // dstAccessMask
                               vk::DependencyFlagBits::eByRegion };                                                  // dependencyFlags

    _renderPass.init( { colorAttachmentDescription, depthAttachmentDescription }, { subpassDescription }, subpassDependencies );
  }

  void Api::initGui( )
  {
    if ( _gui != nullptr )
    {
      initRenderPass( );
      _gui->init( _window->get( ), &_surface, _swapchain.getExtent( ), _swapchain.getImageViews( ) );
    }
  }

  void Api::recordSwapchainCommandBuffers( )
  {
    RX_ASSERT( _pipelinesReady, "Can not record swapchain command buffers because the pipelines have not been initialized yet." );

    if ( _settings->_rayTrace )
    {
      rayTrace( );
    }
    else
    {
      rasterize( );
    }
  }

  void Api::rasterize( )
  {
    std::map<std::string_view, uint32_t> temp;

    for ( auto geometry : _scene->_geometries )
    {
      temp.emplace( geometry->path, 0 );
    }

    for ( const auto& geometryInstance : _scene->_geometryInstances )
    {
      std::shared_ptr<Geometry> it = findGeometry( geometryInstance->geometryIndex );
      RX_ASSERT( it != nullptr, "Could not find model. Did you forget to introduce the renderer to this model using Rayex::setModels( ) after initializing the renderer?" );

      auto it2 = temp.find( it->path );
      if ( it2 != temp.end( ) )
      {
        ++( it2->second );
      }
    }

    // Set up render pass begin info
    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color        = { Util::vec4toArray( _settings->getClearColor( ) ) };
    clearValues[1].depthStencil = vk::ClearDepthStencilValue { 1.0F, 0 };

    // Start recording the swapchain framebuffers
    for ( size_t imageIndex = 0; imageIndex < _swapchainCommandBuffers.get( ).size( ); ++imageIndex )
    {
      _swapchainCommandBuffers.begin( imageIndex );

      _renderPass.begin( _swapchain.getFramebuffer( imageIndex ),
                         _swapchainCommandBuffers.get( imageIndex ),
                         { 0, _swapchain.getExtent( ) },
                         { clearValues[0], clearValues[1] } );

      _swapchainCommandBuffers.get( imageIndex ).bindPipeline( vk::PipelineBindPoint::eGraphics, _rsPipeline.get( ) ); // CMD

      // Dynamic states
      viewport.width  = static_cast<float>( _window->getWidth( ) );
      viewport.height = static_cast<float>( _window->getHeight( ) );

      _swapchainCommandBuffers.get( imageIndex ).setViewport( 0, 1, &viewport ); // CMD

      scissor.extent = _window->getExtent( );

      _swapchainCommandBuffers.get( imageIndex ).setScissor( 0, 1, &scissor ); // CMD

      // Draw models
      uint32_t id = 0;
      for ( const auto& geometryInstance : _scene->_geometryInstances )
      {
        auto geo = findGeometry( geometryInstance->geometryIndex );
        RX_ASSERT( geo != nullptr, "Could not find model. Did you forget to introduce the renderer to this model using Rayex::setModels( ) after initializing the renderer?" );

        uint32_t instanceCount = 1;
        auto it2               = temp.find( geo->path );
        if ( it2 != temp.end( ) )
        {
          instanceCount = it2->second;

          if ( instanceCount == 0 )
          {
            continue;
          }
        }

        _swapchainCommandBuffers.get( imageIndex ).pushConstants( _rsPipeline.getLayout( ),         // layout
                                                                  vk::ShaderStageFlagBits::eVertex, // stageFlags
                                                                  0,                                // offset
                                                                  sizeof( uint32_t ),               // size
                                                                  &id );                            // pValues

        std::array<vk::Buffer, 1> vertexBuffers { _vertexBuffers[geometryInstance->geometryIndex].get( ) };
        std::array<vk::DeviceSize, 1> offsets { 0 };

        _swapchainCommandBuffers.get( imageIndex ).bindVertexBuffers( 0,                     // first binding
                                                                      1,                     // binding count
                                                                      vertexBuffers.data( ), // pBuffers
                                                                      offsets.data( ) );     // pOffsets

        _swapchainCommandBuffers.get( imageIndex ).bindIndexBuffer( _indexBuffers[geometryInstance->geometryIndex].get( ),
                                                                    0, // offset
                                                                    vk::IndexType::eUint32 );

        std::vector<vk::DescriptorSet> descriptorSets = { _rsSceneDescriptorSets[imageIndex] };

        _swapchainCommandBuffers.get( imageIndex ).bindDescriptorSets( vk::PipelineBindPoint::eGraphics, _rsPipeline.getLayout( ),
                                                                       0,                                               // first set
                                                                       static_cast<uint32_t>( descriptorSets.size( ) ), // descriptor set count
                                                                       descriptorSets.data( ),                          // descriptor sets
                                                                       0,                                               // dynamic offset count
                                                                       nullptr );                                       // dynamic offsets

        _swapchainCommandBuffers.get( imageIndex ).drawIndexed( _indexBuffers[geometryInstance->geometryIndex].getCount( ), // index count
                                                                instanceCount,                                              // instance count
                                                                0,                                                          // first index
                                                                0,                                                          // vertex offset
                                                                0 );                                                        // first instance

        ++id;
      }

      _renderPass.end( _swapchainCommandBuffers.get( imageIndex ) );
      _swapchainCommandBuffers.end( imageIndex );
    }
  }

  void Api::rayTrace( )
  {
    auto directionalLightCount = static_cast<uint32_t>( _scene->_directionalLights.size( ) );
    auto pointLightCount       = static_cast<uint32_t>( _scene->_pointLights.size( ) );

    // Start recording the swapchain framebuffers?
    for ( size_t imageIndex = 0; imageIndex < _swapchainCommandBuffers.get( ).size( ); ++imageIndex )
    {
      _swapchainCommandBuffers.begin( imageIndex );

      RayTracePushConstants chitPc = { _settings->getClearColor( ),
                                       components::frameCount,
                                       _settings->getJitterCamSampleRatePerRayGen( ),
                                       _settings->getSsaaSampleRate( ),
                                       static_cast<uint32_t>( _settings->getJitterCamEnabled( ) ), // BAD: bool to uint32_t cast, but bool alignment is only 1 byte
                                       static_cast<uint32_t>( _settings->getSsaaEnabled( ) ),      // BAD: bool to uint32_t cast, but bool alignment is only 1 byte
                                       directionalLightCount,
                                       pointLightCount };

      _swapchainCommandBuffers.get( imageIndex ).pushConstants( _rtBuilder.getPipelineLayout( ),                                                                                   // layout
                                                                vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eClosestHitKHR, // stageFlags
                                                                0,                                                                                                                 // offset
                                                                sizeof( RayTracePushConstants ),                                                                                   // size
                                                                &chitPc );                                                                                                         // pValues

      _swapchainCommandBuffers.get( imageIndex ).bindPipeline( vk::PipelineBindPoint::eRayTracingKHR, _rtBuilder.getPipeline( ) );

      std::vector<vk::DescriptorSet> descriptorSets = { _rtDescriptorSets[imageIndex],
                                                        _rtSceneDescriptorSets[imageIndex],
                                                        _geometryDescriptorSets[imageIndex] };

      _swapchainCommandBuffers.get( imageIndex ).bindDescriptorSets( vk::PipelineBindPoint::eRayTracingKHR, _rtBuilder.getPipelineLayout( ),
                                                                     0,                                               // first set
                                                                     static_cast<uint32_t>( descriptorSets.size( ) ), // descriptor set count
                                                                     descriptorSets.data( ),                          // descriptor sets
                                                                     0,                                               // dynamic offset count
                                                                     nullptr );                                       // dynamic offsets

      _rtBuilder.rayTrace( _swapchainCommandBuffers.get( imageIndex ), _swapchain.getImage( imageIndex ), _swapchain.getExtent( ) );

      _swapchainCommandBuffers.end( imageIndex );
    }
  }

  void Api::initSyncObjects( )
  {
    _imageAvailableSemaphores.resize( maxFramesInFlight );
    _finishedRenderSemaphores.resize( maxFramesInFlight );
    _inFlightFences.resize( maxFramesInFlight );
    _imagesInFlight.resize( components::swapchainImageCount, nullptr );

    for ( size_t i = 0; i < maxFramesInFlight; ++i )
    {
      _imageAvailableSemaphores[i] = vk::Initializer::initSemaphoreUnique( );
      _finishedRenderSemaphores[i] = vk::Initializer::initSemaphoreUnique( );
      _inFlightFences[i]           = vk::Initializer::initFenceUnique( vk::FenceCreateFlagBits::eSignaled );
    }
  }

  void Api::initDescriptorSets( )
  {
    // Create the ray tracing descriptor set layout
    {
      // TLAS
      _rtDescriptors.bindings.add( 0,
                                   vk::DescriptorType::eAccelerationStructureKHR,
                                   vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
                                   1,
                                   vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending | vk::DescriptorBindingFlagBits::ePartiallyBound );
      // Output image
      _rtDescriptors.bindings.add( 1,
                                   vk::DescriptorType::eStorageImage,
                                   vk::ShaderStageFlagBits::eRaygenKHR,
                                   1,
                                   vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending | vk::DescriptorBindingFlagBits::ePartiallyBound );

      _rtDescriptors.layout = _rtDescriptors.bindings.initLayoutUnique( );
      _rtDescriptors.pool   = _rtDescriptors.bindings.initPoolUnique( components::swapchainImageCount );
      _rtDescriptorSets     = vk::Initializer::initDescriptorSetsUnique( _rtDescriptors.pool, _rtDescriptors.layout );
    }

    // RT Scene descriptor set layout.
    {
      // Camera uniform buffer
      _rtSceneDescriptors.bindings.add( 0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eRaygenKHR );
      // Directional lights storage buffer
      _rtSceneDescriptors.bindings.add( 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR );
      // Point lights storage buffer
      _rtSceneDescriptors.bindings.add( 2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR );
      // Scene description buffer
      _rtSceneDescriptors.bindings.add( 3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR );

      _rtSceneDescriptors.layout = _rtSceneDescriptors.bindings.initLayoutUnique( );
      _rtSceneDescriptors.pool   = _rtSceneDescriptors.bindings.initPoolUnique( static_cast<uint32_t>( _settings->_maxGeometry ) * components::swapchainImageCount );
      _rtSceneDescriptorSets     = vk::Initializer::initDescriptorSetsUnique( _rtSceneDescriptors.pool, _rtSceneDescriptors.layout );
    }

    // RS Scene descriptor set layout.
    {
      // Camera uniform buffer
      _rsSceneDescriptors.bindings.add( 0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex );
      // Lights storage buffer
      _rsSceneDescriptors.bindings.add( 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment );
      // Scene description buffer
      _rsSceneDescriptors.bindings.add( 2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex );

      _rsSceneDescriptors.layout = _rsSceneDescriptors.bindings.initLayoutUnique( );
      _rsSceneDescriptors.pool   = _rsSceneDescriptors.bindings.initPoolUnique( components::swapchainImageCount );
      _rsSceneDescriptorSets     = vk::Initializer::initDescriptorSetsUnique( _rsSceneDescriptors.pool, _rsSceneDescriptors.layout );
    }

    // Geometry descriptor set layout.
    {
      // Vertex buffers
      _geometryDescriptors.bindings.add( 0,
                                         vk::DescriptorType::eStorageBuffer,
                                         vk::ShaderStageFlagBits::eClosestHitKHR,
                                         _settings->_maxGeometry,
                                         vk::DescriptorBindingFlagBits::eUpdateAfterBind );

      // Index buffers
      _geometryDescriptors.bindings.add( 1,
                                         vk::DescriptorType::eStorageBuffer,
                                         vk::ShaderStageFlagBits::eClosestHitKHR,
                                         _settings->_maxGeometry,
                                         vk::DescriptorBindingFlagBits::eUpdateAfterBind );

      // Mesh buffers
      _geometryDescriptors.bindings.add( 2,
                                         vk::DescriptorType::eStorageBuffer,
                                         vk::ShaderStageFlagBits::eClosestHitKHR,
                                         _settings->_maxGeometry,
                                         vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::eVariableDescriptorCount );

      _geometryDescriptors.layout = _geometryDescriptors.bindings.initLayoutUnique( vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool );
      _geometryDescriptors.pool   = _geometryDescriptors.bindings.initPoolUnique( static_cast<uint32_t>( _settings->_maxGeometry ) * components::swapchainImageCount, vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind );
      _geometryDescriptorSets     = vk::Initializer::initDescriptorSetsUnique( _geometryDescriptors.pool, _geometryDescriptors.layout );
    }
  }

  void Api::updateSettings( )
  {
    if ( _settings->_refreshPipeline )
    {
      _settings->_refreshPipeline = false;

      components::device.waitIdle( );

#ifdef RX_COPY_ASSETS
      // Copies shader resources to binary output directory. This way a shader can be changed during runtime.
      // Make sure only to edit the ones in /assets/shaders and not in /build/bin/debug/assets/shaders as the latter gets overridden.
      RX_INFO( "Copying shader resources to binary output directory. " );
      std::filesystem::copy( RX_ASSETS_PATH "shaders", RX_PATH_TO_LIBRARY "shaders", std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive );
#endif

      initPipelines( );
      _rtBuilder.createShaderBindingTable( );
    }

    if ( _settings->_refreshSwapchain )
    {
      _settings->_refreshSwapchain = false;

      recreateSwapchain( );
    }
  }

  void Api::updateUniformBuffers( )
  {
    uint32_t imageIndex = _swapchain.getCurrentImageIndex( );

    // Upload camera.
    if ( _camera != nullptr )
    {
      if ( _camera->_updateView )
      {
        cameraUbo.view        = _camera->getViewMatrix( );
        cameraUbo.viewInverse = _camera->getViewInverseMatrix( );

        _camera->_updateView = false;
      }

      if ( _camera->_updateProj )
      {
        cameraUbo.projection        = _camera->getProjectionMatrix( );
        cameraUbo.projectionInverse = _camera->getProjectionInverseMatrix( );

        _camera->_updateProj = false;
      }

      cameraUbo.position = glm::vec4( _camera->getPosition( ), 1.0F );
    }

    _cameraUniformBuffer.upload<CameraUbo>( imageIndex, cameraUbo );
  }

  void Api::updateSceneDescriptors( )
  {
    vk::DescriptorBufferInfo rtInstancesInfo( _geometryInstancesBuffer.get( ),
                                              0,
                                              VK_WHOLE_SIZE );

    vk::DescriptorBufferInfo directionalLightsInfo( _directionalLightsBuffer.get( ),
                                                    0,
                                                    VK_WHOLE_SIZE );

    vk::DescriptorBufferInfo pointLightsInfo( _pointLightsBuffer.get( ),
                                              0,
                                              VK_WHOLE_SIZE );

    // Update RT scene descriptor sets.
    _rtSceneDescriptors.bindings.write( _rtSceneDescriptorSets, 0, _cameraUniformBuffer._bufferInfos );
    _rtSceneDescriptors.bindings.write( _rtSceneDescriptorSets, 1, &directionalLightsInfo );
    _rtSceneDescriptors.bindings.write( _rtSceneDescriptorSets, 2, &pointLightsInfo );
    _rtSceneDescriptors.bindings.write( _rtSceneDescriptorSets, 3, &rtInstancesInfo );

    _rtSceneDescriptors.bindings.update( );

    // Update RS scene descriptor sets.
    _rsSceneDescriptors.bindings.write( _rsSceneDescriptorSets, 0, _cameraUniformBuffer._bufferInfos );
    _rsSceneDescriptors.bindings.write( _rsSceneDescriptorSets, 1, &directionalLightsInfo );
    _rsSceneDescriptors.bindings.write( _rsSceneDescriptorSets, 2, &rtInstancesInfo );
    _rsSceneDescriptors.bindings.update( );
  }

  void Api::updateRayTracingModelData( )
  {
    RX_ASSERT( _scene->_geometries.size( ) <= _settings->_maxGeometry, "Can not bind more than ", _settings->_maxGeometry, " geometries." );
    RX_ASSERT( _meshBuffers.size( ) <= _settings->_maxGeometry, "Can not bind more than ", _settings->_maxGeometry, " meshes." );

    // Update RT model data.
    std::vector<vk::DescriptorBufferInfo> vertexBufferInfos;
    vertexBufferInfos.reserve( _vertexBuffers.size( ) );
    for ( const auto& vertexBuffer : _vertexBuffers )
    {
      vk::DescriptorBufferInfo vertexDataBufferInfo( vertexBuffer.get( ),
                                                     0,
                                                     VK_WHOLE_SIZE );

      vertexBufferInfos.push_back( vertexDataBufferInfo );
    }

    std::vector<vk::DescriptorBufferInfo> indexBufferInfos;
    indexBufferInfos.reserve( _indexBuffers.size( ) );
    for ( const auto& indexBuffer : _indexBuffers )
    {
      vk::DescriptorBufferInfo indexDataBufferInfo( indexBuffer.get( ),
                                                    0,
                                                    VK_WHOLE_SIZE );

      indexBufferInfos.push_back( indexDataBufferInfo );
    }

    std::vector<vk::DescriptorBufferInfo> meshBufferInfos;
    meshBufferInfos.reserve( _meshBuffers.size( ) );
    for ( const auto& meshBuffer : _meshBuffers )
    {
      vk::DescriptorBufferInfo meshDataBufferInfo( meshBuffer.get( ),
                                                   0,
                                                   VK_WHOLE_SIZE );

      meshBufferInfos.push_back( meshDataBufferInfo );
    }

    _geometryDescriptors.bindings.writeArray( _geometryDescriptorSets, 0, vertexBufferInfos.data( ) );
    _geometryDescriptors.bindings.writeArray( _geometryDescriptorSets, 1, indexBufferInfos.data( ) );
    _geometryDescriptors.bindings.writeArray( _geometryDescriptorSets, 2, meshBufferInfos.data( ) );

    _geometryDescriptors.bindings.update( );
  }

  auto Api::findGeometry( uint32_t geometryIndex ) -> std::shared_ptr<Geometry>
  {
    for ( std::shared_ptr<Geometry> geometry : _scene->_geometries )
    {
      if ( geometry->geometryIndex == geometryIndex )
      {
        return geometry;
      }
    }

    RX_FATAL( "Could not find geometry in scene." );
    return nullptr;
  }
} // namespace RAYEX_NAMESPACE
