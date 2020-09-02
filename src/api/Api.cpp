#include "Api.hpp"
#include "Components.hpp"
#include "Initializers.hpp"
#include "Helpers.hpp"
#include "Destructors.hpp"
#include "Settings.hpp"

namespace rx
{
  const std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };
#ifdef RX_DEBUG
  std::vector<const char*> extensions = { "VK_KHR_get_physical_device_properties2", "VK_EXT_debug_utils" };
#elif
  std::vector<const char*> extensions = { "VK_KHR_get_physical_device_properties2" };
#endif

  std::vector<const char*> deviceExtensions =
  {
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
    VK_KHR_MAINTENANCE3_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_EXTENSION_NAME
  };

  size_t currentFrame = 0;

  // Defines the maximum amount of frames that will be processed concurrently.
  const size_t maxFramesInFlight = 2;

  Api::Api( std::shared_ptr<WindowBase> window, std::shared_ptr<CameraBase> camera ) :
    m_window( window ),
    m_camera( camera ),
    m_gui( nullptr )
  { }

  Api::Api( std::shared_ptr<WindowBase> window, std::shared_ptr<GuiBase> gui, std::shared_ptr<CameraBase> camera ) :
    m_window( window ),
    m_camera( camera ),
    m_gui( gui )
  { }

  Api::~Api( )
  {
    clean( );

    // Gui needs to be destroyed manually, as RAII destruction will not be possible.
    if ( m_gui != nullptr )
      m_gui->destroy( );
  }

  void Api::setGui( const std::shared_ptr<GuiBase> gui )
  {
    m_gui = gui;
  }

  void Api::init( )
  {
    m_geometryNodes.reserve( g_maxGeometryNodes );
    m_textures.reserve( g_maxTextures );
    m_lightNodes.reserve( g_maxLightNodes );

    // Instance
    m_instance.init( layers, extensions );

    // Debug messenger.
    m_debugMessenger.init( vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );
    
    // Surface
    m_surface.init( );

    // Physical device
    m_physicalDevice.init( );

    // Reassess the support of the preferred surface settings.
    m_surface.checkSettingSupport( );

    // Queues
    m_queues.init( );

    // Logical device
    m_device.init( deviceExtensions );

    // Retrieve all queue handles.
    m_queues.retrieveHandles( );

    // Render pass
    initRenderPass( );

    // Swapchain
    m_swapchain.init( &m_surface, m_renderPass.get( ) );

    // Descriptor set layouts
    initDescriptorSetLayouts( );

    // Descriptor pool
    m_descriptorPool = vk::Initializer::createDescriptorPoolUnique( vk::Helper::getPoolSizes( m_descriptorSetLayout.getBindings( ) ), static_cast<uint32_t>( g_maxGeometryNodes ) * g_swapchainImageCount );

    // Ray tracing descriptor pool
    m_rtDescriptorPool = vk::Initializer::createDescriptorPoolUnique( vk::Helper::getPoolSizes( m_rtDescriptorSetLayout.getBindings( ) ), static_cast<uint32_t>( g_maxGeometryNodes ) * g_swapchainImageCount );
    
    // Ray tracing descriptor set
    m_rtDescriptorSets.init( m_rtDescriptorPool.get( ), g_swapchainImageCount, std::vector<vk::DescriptorSetLayout>{ g_swapchainImageCount, m_rtDescriptorSetLayout.get( ) } );

    // Uniform buffer for camera
    m_cameraUniformBuffer.init<CameraUbo>( g_swapchainImageCount );

    // Pipeline
    m_rtPipeline.init( vk::Rect2D( 0, { m_swapchain.getExtent( ).width, m_swapchain.getExtent( ).height } ), // scissor
                       { m_rtDescriptorSetLayout.get( ), m_descriptorSetLayout.get( ) } );                   // descriptorSetLayouts

    // Command pools
    m_graphicsCmdPool = vk::Initializer::createCommandPoolUnique( g_graphicsFamilyIndex, vk::CommandPoolCreateFlagBits::eResetCommandBuffer );
    g_graphicsCmdPool = m_graphicsCmdPool.get( );

    m_transferCmdPool = vk::Initializer::createCommandPoolUnique( g_transferFamilyIndex, { } );
    g_transferCmdPool = m_transferCmdPool.get( );

    // Ray tracing
    m_rayTracingBuilder.init( );
    m_rayTracingBuilder.createStorageImage( m_swapchain.getExtent( ) );
    m_rayTracingBuilder.createShaderBindingTable( m_rtPipeline.get( ) );
    
    // Swapchain command buffers
    m_swapchainCommandBuffers.init( m_graphicsCmdPool.get( ), g_swapchainImageCount, vk::CommandBufferUsageFlagBits::eRenderPassContinue );

    // GUI
    initGui( );

    // Record swapchain command buffers.
    recordSwapchainCommandBuffers( );

    // Create fences and semaphores.
    initSyncObjects( );

    // Make sure swapchain images are presentable in case they were not transitioned automatically.
    m_swapchain.setImageLayout( vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR );


    RX_LOG( "Finished API initialization." );
  }

  bool Api::update( )
  {
    if ( Settings::refresh( ) )
    {
      // Trigger swapchain / pipeline recreation
    }

    uint32_t imageIndex = m_swapchain.getCurrentImageIndex( );

    // Update camera
    // TODO: only update when necessary.
    glm::mat4 view = m_camera->getViewMatrix( );
    glm::mat4 viewInverse = glm::inverse( view );

    glm::mat4 proj = m_camera->getProjectionMatrix( );
    glm::mat4 projInverse = glm::inverse( proj );

    CameraUbo camUbo { view, proj, viewInverse, projInverse };
    m_cameraUniformBuffer.upload<CameraUbo>( imageIndex, camUbo );

    return true;
  }

  bool Api::prepareFrame( )
  {
    // Wait for the current frame's fences.
    g_device.waitForFences( 1, &m_inFlightFences[currentFrame].get( ), VK_TRUE, UINT64_MAX );

    // If the window is minimized then simply do not render anything anymore.
    if ( m_window->minimized( ) )
      return true;

    // If the window size has changed the swapchain has to be recreated.
    if ( m_window->changed( ) || m_recreateSwapchain )
    {
      m_recreateSwapchain = false;
      recreateSwapchain( );
      return true;
    }

    m_swapchain.acquireNextImage( m_imageAvailableSemaphores[currentFrame].get( ), nullptr );
  }

  bool Api::submitFrame( )
  {
    uint32_t imageIndex = m_swapchain.getCurrentImageIndex( );
      
    // Check if a previous frame is using the current image.
    if ( m_imagesInFlight[imageIndex] )
      g_device.waitForFences( 1, &m_imagesInFlight[currentFrame], VK_TRUE, UINT64_MAX );

    // This will mark the current image to be in use by this frame.
    m_imagesInFlight[imageIndex] = m_inFlightFences[currentFrame].get( );

    // Add GUI command buffer to swapchain command buffer if GUI is enabled.
    std::vector<vk::CommandBuffer> commandBuffers = { m_swapchainCommandBuffers.get( )[imageIndex] };
    if ( m_gui != nullptr )
      commandBuffers.push_back( m_gui->getCommandBuffer( imageIndex ) );

    // Reset the signaled state of the current frame's fence to the unsignaled one.
    g_device.resetFences( 1, &m_inFlightFences[currentFrame].get( ) );

    // Submits / executes the current image's / framebuffer's command buffer.
    auto pWaitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto submitInfo = vk::Helper::getSubmitInfo( m_imageAvailableSemaphores[currentFrame].get( ), m_finishedRenderSemaphores[currentFrame].get( ), commandBuffers, pWaitDstStageMask );
    g_graphicsQueue.submit( submitInfo, m_inFlightFences[currentFrame].get( ) );

    // Tell the presentation engine that the current image is ready.
    auto presentInfo = vk::Helper::getPresentInfoKHR( m_finishedRenderSemaphores[currentFrame].get( ), imageIndex );
    g_graphicsQueue.presentKHR( presentInfo );

    currentFrame = ( currentFrame + 1 ) % maxFramesInFlight;

    return true;
  }

  bool Api::render( )
  {
    if ( m_gui != nullptr )
    {
      m_gui->newFrame( );
      m_gui->render( );
      m_gui->endRender( );
    }

    if ( prepareFrame( ) )
      return true;

    if ( m_gui != nullptr )
      m_gui->renderDrawData( m_swapchain.getCurrentImageIndex( ) );

    if ( submitFrame( ) )
      return true;

    return true;
  }

  void Api::pushNode( const std::shared_ptr<Node> node, bool record )
  {
    if ( dynamic_cast<GeometryNode*>( node.get( ) ) )
    {
      auto ptr = std::dynamic_pointer_cast<GeometryNode>( node );

      auto it = m_models.find( ptr->m_modelPath );
      if ( it == m_models.end( ) )
        m_models.insert( { ptr->m_modelPath, std::make_shared<Model>( ptr->m_modelPath ) } );

      m_geometryNodes.push_back( ptr );

      // Handle the node's texture.
      auto texturePaths = ptr->m_material.getTextures( );

      for ( const auto& texturePath : texturePaths )
      {
        auto it = m_textures.find( texturePath );
        // Texture does not exist already. It will be created.
        if ( it == m_textures.end( ) )
        {
          m_textures.insert( { texturePath, std::make_shared<Texture>( texturePath ) } );
        }
      }

      if ( record )
        initModel( ptr );
    }
    else if ( dynamic_cast<LightNode*>( node.get( ) ) )
    {
      auto lightNodePtr = std::dynamic_pointer_cast<LightNode>( node );

      m_lightNodes.push_back( lightNodePtr );
    }

    if ( record )
    {
      m_swapchainCommandBuffers.reset( );
      recordSwapchainCommandBuffers( );
    }
  }

  void Api::setNodes( const std::vector<std::shared_ptr<Node>>& nodes )
  {
    m_geometryNodes.clear( );
    m_geometryNodes.reserve( g_maxGeometryNodes );

    for ( const auto& node : nodes )
      pushNode( node );

    m_swapchainCommandBuffers.reset( );
    recordSwapchainCommandBuffers( );
  }

  void Api::clean( )
  {
    g_device.waitIdle( );
  }

  void Api::recreateSwapchain( )
  {
    RX_LOG( "Recreating swapchain." );

    g_device.waitIdle( );

    // Clean up existing swapchain and dependencies.
    m_swapchainCommandBuffers.free( );
    m_swapchain.destroy( );

    // Recreating the swapchain.
    m_swapchain.init( &m_surface, m_renderPass.get( ) );

    // Recreate storage image with the new swapchain image size and update the ray tracing descriptor set to use the new storage image view.
    m_rayTracingBuilder.createStorageImage( m_swapchain.getExtent( ) );
    m_rtDescriptorSets.update( m_rayTracingBuilder.getTlas( ).as.as, 
                               m_rayTracingBuilder.getStorageImageView( ), 
                               m_cameraUniformBuffer.getRaw( ) );

    // Swapchain command buffers
    m_swapchainCommandBuffers.init( m_graphicsCmdPool.get( ), g_swapchainImageCount, vk::CommandBufferUsageFlagBits::eRenderPassContinue );
    recordSwapchainCommandBuffers( );

    if ( m_gui != nullptr )
      m_gui->recreate( m_swapchain.getExtent( ), m_swapchain.getImageViews( ) );

    // Update the camera screen size to avoid image stretching.
    auto screenSize = m_swapchain.getExtent( );
    m_camera->setSize( screenSize.width, screenSize.height );
    
    RX_LOG( "Finished swapchain recreation." );
  }

  void Api::initRenderPass( )
  {
    auto colorAttachmentDescription = vk::Helper::getAttachmentDescription( m_surface.getFormat( ), m_gui != nullptr );

    vk::AttachmentReference colorAttachmentReference( 0,                                          // attachment
                                                      vk::ImageLayout::eColorAttachmentOptimal ); // layout

    auto depthAttachmentDescription = vk::Helper::getDepthAttachmentDescription( getSupportedDepthFormat( g_physicalDevice ) );

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

    m_renderPass.init( { colorAttachmentDescription, depthAttachmentDescription }, { subpassDescription }, subpassDependencies );
  }

  void Api::initModel( const std::shared_ptr<GeometryNode> node )
  {
    // TODO: add handling for missing textures, models etc.

    auto it = m_models.find( node->m_modelPath );
    auto model = it->second;
    if ( !model->m_initialized )
    {
      model->m_vertexBuffer.init( model->m_vertices );
      model->m_indexBuffer.init( model->m_indices );
      model->m_descriptorSets.init( m_descriptorPool.get( ), g_swapchainImageCount, std::vector<vk::DescriptorSetLayout>{ g_swapchainImageCount, m_descriptorSetLayout.get( ) } );

      model->m_initialized = true;
    }

    node->m_rtInstance.m_modelIndex = model->m_index;

    // TODO: Try to call this as few times as possible.   
    m_rayTracingBuilder.createBottomLevelAS( vk::Helper::unpack( m_models ) );
    m_rayTracingBuilder.createTopLevelAS( m_geometryNodes );

    // Update ray tracing descriptor set.
    m_rtDescriptorSets.update( m_rayTracingBuilder.getTlas( ).as.as, m_rayTracingBuilder.getStorageImageView( ), m_cameraUniformBuffer.getRaw( ) );

    // Update second descriptor set.
    auto diffuseIter = m_textures.find( node->m_material.m_diffuseTexture );
    model->m_descriptorSets.update( diffuseIter->second->getImageView( ), diffuseIter->second->getSampler( ), model->m_vertexBuffer.get( ), model->m_indexBuffer.get( ) );
  }

  void Api::initGui( )
  {
    if ( m_gui != nullptr )
    {
      initRenderPass( );
      m_gui->init( &m_surface, m_swapchain.getExtent( ), m_swapchain.getImageViews( ) );
    }
  }

  void Api::recordSwapchainCommandBuffers( )
  {
    RX_LOG( "Started swapchain command buffer recording." );

    // Start recording the swapchain framebuffers
    for ( size_t imageIndex = 0; imageIndex < m_swapchainCommandBuffers.get( ).size( ); ++imageIndex )
    {
      m_swapchainCommandBuffers.begin( imageIndex );

      m_rtPipeline.bind( m_swapchainCommandBuffers.get( imageIndex ) );
        
      for ( const auto& node : m_geometryNodes )
      {
        auto iter = m_models.find( node->m_modelPath );
        RX_ASSERT( ( iter->second != nullptr ), "Can not find model" );

        std::vector<vk::DescriptorSet> descriptorSets = { m_rtDescriptorSets.get( imageIndex ), iter->second->m_descriptorSets.get( imageIndex ) };

        m_swapchainCommandBuffers.get( imageIndex ).bindDescriptorSets( vk::PipelineBindPoint::eRayTracingKHR,
                                                                        m_rtPipeline.getLayout( ),
                                                                        0,                                                           // first set
                                                                        static_cast<uint32_t>( descriptorSets.size( ) ),             // descriptor set count
                                                                        descriptorSets.data( ),                                      // descriptor sets
                                                                        0,                                                           // dynamic offset count
                                                                        nullptr );                                                   // dynamic offsets 

        m_rayTracingBuilder.rayTrace( m_swapchainCommandBuffers.get( imageIndex ), m_swapchain.getImage( imageIndex ), m_window->getExtent( ) );
      }

      m_swapchainCommandBuffers.end( imageIndex );
    }
  }

  void Api::initSyncObjects( )
  {
    m_imageAvailableSemaphores.resize( maxFramesInFlight );
    m_finishedRenderSemaphores.resize( maxFramesInFlight );
    m_inFlightFences.resize( maxFramesInFlight );
    m_imagesInFlight.resize( g_swapchainImageCount, nullptr );

    for ( size_t i = 0; i < maxFramesInFlight; ++i )
    {
      m_imageAvailableSemaphores[i] = vk::Initializer::createSemaphoreUnique( );
      m_finishedRenderSemaphores[i] = vk::Initializer::createSemaphoreUnique( );
      m_inFlightFences[i] = vk::Initializer::createFenceUnique( );
    }
  }

  void Api::initDescriptorSetLayouts( )
  {
    // Create the ray tracing descriptor set layout

    // TLAS
    vk::DescriptorSetLayoutBinding tlasBinding( 0,                                                                             // binding
                                                vk::DescriptorType::eAccelerationStructureKHR,                                 // descriptorType
                                                1,                                                                             // descriptorCount
                                                vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR, // stageFlags
                                                nullptr );                                                                     // pImmutableSamplers

    // Output imagey
    vk::DescriptorSetLayoutBinding outputImageBinding( 1,                                   // binding
                                                       vk::DescriptorType::eStorageImage,   // descriptorType
                                                       1,                                   // descriptorCount
                                                       vk::ShaderStageFlagBits::eRaygenKHR, // stageFlags
                                                       nullptr );                           // pImmutableSamplers
    // Uniform buffer
    vk::DescriptorSetLayoutBinding cameraUniformBufferBinding( 2,                                   // binding
                                                               vk::DescriptorType::eUniformBuffer,  // descriptorType
                                                               1,                                   // descriptorCount
                                                               vk::ShaderStageFlagBits::eRaygenKHR, // stageFlags
                                                               nullptr );                           // pImmutableSamplers

    std::vector<vk::DescriptorSetLayoutBinding> rtBindings = { tlasBinding, outputImageBinding, cameraUniformBufferBinding };
    m_rtDescriptorSetLayout.init( rtBindings );

    // Create the descriptor set layout for models

    // Texture image
    vk::DescriptorSetLayoutBinding textureBinding( 0,                                                                            // binding
                                                   vk::DescriptorType::eCombinedImageSampler,                                    // descriptorType
                                                   1,                                                                            // descriptorCount
                                                   vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eClosestHitKHR, // stageFlags
                                                   nullptr );                                                                    // pImmutableSamplers

    // Vertex buffer
    vk::DescriptorSetLayoutBinding vertexBufferBinding( 1,                                       // binding
                                                        vk::DescriptorType::eStorageBuffer,      // descriptorType
                                                        1,                                       // descriptorCount
                                                        vk::ShaderStageFlagBits::eClosestHitKHR, // stageFlags
                                                        nullptr );                               // pImmutableSamplers

    // Index buffer
    vk::DescriptorSetLayoutBinding indexBufferBinding( 2,                                       // binding
                                                       vk::DescriptorType::eStorageBuffer,      // descriptorType
                                                       1,                                       // descriptorCount
                                                       vk::ShaderStageFlagBits::eClosestHitKHR, // stageFlags
                                                       nullptr );                               // pImmutableSamplers

    std::vector<vk::DescriptorSetLayoutBinding> bindings = { textureBinding, vertexBufferBinding, indexBufferBinding };
    m_descriptorSetLayout.init( bindings );
  }
}
