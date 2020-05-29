#include "Api.hpp"

namespace RX
{
  // Defines the maximum amount of frames that will be processed concurrently.
  const size_t maxFramesInFlight = 2;

  RenderPassInfo renderPassInfo{ };
  SwapchainInfo swapchainInfo{ };
  ShaderInfo vertexShaderInfo{ };
  ShaderInfo fragmentShaderInfo{ };
  PipelineInfo pipelineInfo{ };
  CommandBufferInfo commandBufferInfo{ };
  RenderPassBeginInfo renderPassBeginInfo{ };
  UniformBufferInfo uniformBufferInfo{ };
  DescriptorPoolInfo descriptorPoolInfo{ };
  DescriptorSetInfo descriptorSetInfo{ };
  
  Api::Api(std::shared_ptr<Window> window) :
    m_window(window) { }

  Api::~Api()
  {
    clean();
  }

  void Api::initialize()
  {
    // Instance
    InstanceInfo instanceInfo{ };
    instanceInfo.window = m_window;
    instanceInfo.layers = { "VK_LAYER_KHRONOS_validation" };
    instanceInfo.extensions = { "VK_EXT_debug_utils", "VK_KHR_get_physical_device_properties2" };

    m_instance.initialize(instanceInfo);

    // Debug messenger
    DebugMessengerInfo debugMessengerInfo{ };
    debugMessengerInfo.instance = m_instance.get();

    m_debugMessenger.initialize(debugMessengerInfo);

    // Surface
    SurfaceInfo surfaceInfo{ };
    surfaceInfo.window = m_window;
    surfaceInfo.instance = m_instance.get();

    m_surface.initialize(surfaceInfo);

    // Physical device
    PhysicalDeviceInfo physicalDeviceInfo{ };
    physicalDeviceInfo.instance = m_instance.get();
    physicalDeviceInfo.surface = m_surface.get();
    
    m_physicalDevice.initialize(physicalDeviceInfo);

    // Reassess the support of the preferred surface settings.
    m_surface.checkSettingSupport(m_physicalDevice.get());

    // Queues
    QueuesInfo queuesInfo{ };
    queuesInfo.physicalDevice = m_physicalDevice.get();
    queuesInfo.surface = m_surface.get();

    m_queues.initialize(queuesInfo);

    // Device
    DeviceInfo deviceInfo{ };
    deviceInfo.physicalDevice = m_physicalDevice.get();
    deviceInfo.queueFamilyIndices = m_queues.getQueueFamilyIndices();
    deviceInfo.extensions = { "VK_KHR_get_memory_requirements2", "VK_EXT_descriptor_indexing", "VK_KHR_buffer_device_address",  "VK_KHR_deferred_host_operations", "VK_KHR_pipeline_library", "VK_KHR_ray_tracing"};

    m_device.initialize(deviceInfo);

    // Retrieve all queue handles.
    m_queues.retrieveAllHandles(m_device.get());

    // Render pass
    renderPassInfo.physicalDevice = m_physicalDevice.get();
    renderPassInfo.device = m_device.get();
    renderPassInfo.surfaceFormat = m_surface.getInfo().format;
    renderPassInfo.depthFormat = getSupportedDepthFormat(m_physicalDevice.get());

    m_renderPass.initialize(renderPassInfo);

    // Swapchain
    swapchainInfo.window = m_window;
    swapchainInfo.physicalDevice = m_physicalDevice.get();
    swapchainInfo.device = m_device.get();
    swapchainInfo.surface = m_surface.get();
    swapchainInfo.surfaceFormat = m_surface.getInfo().format;
    swapchainInfo.surfaceColorSpace = m_surface.getInfo().colorSpace;
    swapchainInfo.surfacePresentMode = m_surface.getInfo().presentMode;
    swapchainInfo.surfaceCapabilities = m_surface.getInfo().capabilities;
    swapchainInfo.queueFamilyIndices = m_queues.getQueueFamilyIndices();
    swapchainInfo.renderPass = m_renderPass.get();

    m_swapchain.initialize(swapchainInfo);
    m_swapchain.initImageViews();

    // Shaders
    Shader vs;
    vertexShaderInfo.fullPath = RX_SHADER_PATH "simple3D.vert";
    vertexShaderInfo.device = m_device.get();
    vertexShaderInfo.binding = 0;
    vertexShaderInfo.descriptorCount = 1;
    vertexShaderInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vertexShaderInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    vs.initialize(vertexShaderInfo);

    Shader fs; 
    fragmentShaderInfo.fullPath = RX_SHADER_PATH "simple3D.frag";
    fragmentShaderInfo.device = m_device.get();
    fragmentShaderInfo.binding = 1;
    fragmentShaderInfo.descriptorCount = 1;
    fragmentShaderInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    fragmentShaderInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    fs.initialize(fragmentShaderInfo);
    
    // Descriptor Set Layout
    DescriptorSetLayoutInfo descriptorSetLayoutInfo{ };
    descriptorSetLayoutInfo.device = m_device.get();
    descriptorSetLayoutInfo.layoutBindings = { vs.getDescriptorSetLayoutBinding(), fs.getDescriptorSetLayoutBinding() };

    m_descriptorSetLayout.initialize(descriptorSetLayoutInfo);
    
    // Graphics pipeline
    pipelineInfo.device = m_device.get();
    pipelineInfo.renderPass = m_renderPass.get();
    pipelineInfo.viewport = { 0.0f, 0.0f, static_cast<float>(m_swapchain.getInfo().extent.width), static_cast<float>(m_swapchain.getInfo().extent.height), 0.0f, 1.0f };
    pipelineInfo.scissor = { 0, 0, m_swapchain.getInfo().extent.width, m_swapchain.getInfo().extent.height };
    pipelineInfo.vertexShader = vs.get();
    pipelineInfo.fragmentShader = fs.get();
    pipelineInfo.descriptorSetLayout = m_descriptorSetLayout.get();

    m_pipeline.initialize(pipelineInfo);
    
    // Command pool
    CommandPoolInfo commandPoolInfo{ };
    commandPoolInfo.device = m_device.get();
    commandPoolInfo.queueFamilyIndex = m_queues.getGraphicsFamilyIndex();

    m_graphicsCmdPool.initialize(commandPoolInfo);

    // Swapchain depth image
    m_swapchain.initDepthImage();
    m_swapchain.initDepthImageView();

    // Swapchain framebuffers
    m_swapchain.initFramebuffers();

    TextureInfo textureInfo{ };
    textureInfo.physicalDevice = m_physicalDevice.get();
    textureInfo.device = m_device.get();
    textureInfo.queue = m_queues.getGraphicsQueue();
    textureInfo.commandPool = m_graphicsCmdPool.get();

    VertexBufferInfo vertexBufferInfo{ };
    vertexBufferInfo.physicalDevice = m_physicalDevice.get();
    vertexBufferInfo.device = m_device.get();
    vertexBufferInfo.commandPool = m_graphicsCmdPool.get();
    vertexBufferInfo.queue = m_queues.getGraphicsQueue();

    IndexBufferInfo indexBufferInfo{ };
    indexBufferInfo.physicalDevice = m_physicalDevice.get();
    indexBufferInfo.device = m_device.get();
    indexBufferInfo.commandPool = m_graphicsCmdPool.get();
    indexBufferInfo.queue = m_queues.getGraphicsQueue();

    uniformBufferInfo.physicalDevice = m_physicalDevice.get();
    uniformBufferInfo.device = m_device.get();
    uniformBufferInfo.swapchainImagesCount = m_swapchain.getInfo().images.size();

    descriptorPoolInfo.device = m_device.get();
    descriptorPoolInfo.swapchainImagesCount = m_swapchain.getInfo().images.size();
    descriptorPoolInfo.types = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
    descriptorPoolInfo.maxSets = descriptorPoolInfo.swapchainImagesCount; // TODO: Set this to the number of models in your scene.

    descriptorSetInfo.device = m_device.get();
    descriptorSetInfo.swapchainImagesCount = m_swapchain.getInfo().images.size();
    descriptorSetInfo.descriptorSetLayout = m_descriptorSetLayout.get();

    for (std::shared_ptr<Model> model : m_models)
    {
      model->initialize();

      textureInfo.path = model->m_pathToTexture;
      model->m_texture.initialize(textureInfo);

      vertexBufferInfo.vertices = model->m_vertices;
      model->m_vertexBuffer.initialize(vertexBufferInfo);

      indexBufferInfo.indices = model->m_indices;
      model->m_indexBuffer.initialize(indexBufferInfo);

      uniformBufferInfo.uniformBufferObject = model->getUbo();
      model->m_uniformBuffers.initialize(uniformBufferInfo);

      model->m_descriptorPool.initialize(descriptorPoolInfo);
      
      descriptorSetInfo.descriptorPool = model->m_descriptorPool.get();
      descriptorSetInfo.textureSampler = model->m_texture.getSampler();
      descriptorSetInfo.textureImageView = model->m_texture.getImageView();
      descriptorSetInfo.uniformBuffers = model->m_uniformBuffers.getRaw();

      model->m_descriptorSets.initialize(descriptorSetInfo);
    }

    // Command buffers for swapchain
    commandBufferInfo.device = m_device.get();
    commandBufferInfo.commandPool = m_graphicsCmdPool.get();
    commandBufferInfo.commandBufferCount = m_swapchain.getInfo().framebuffers.size();
    commandBufferInfo.usageFlags = 0;

    m_swapchainCmdBuffers.initialize(commandBufferInfo);
    record();

    // Set up the synchronization objects.
    m_imageAvailableSemaphores.resize(maxFramesInFlight);
    m_finishedRenderSemaphores.resize(maxFramesInFlight);
    m_inFlightFences.resize(maxFramesInFlight);
    m_imagesInFlight.resize(m_swapchain.getInfo().images.size(), VK_NULL_HANDLE);

    SemaphoreInfo semaphoreInfo{ };
    semaphoreInfo.device = m_device.get();

    FenceInfo fenceInfo{ };
    fenceInfo.device = m_device.get();

    for (size_t i = 0; i < maxFramesInFlight; ++i)
    {
      m_imageAvailableSemaphores[i].initialize(semaphoreInfo);
      m_finishedRenderSemaphores[i].initialize(semaphoreInfo);
      m_inFlightFences[i].initialize(fenceInfo);
    }

    RX_LOG("Finished API initialization.");
  }

  bool Api::update()
  {
    return true;
  }

  bool Api::render()
  {
    static size_t currentFrame = 0;

    // Wait for the current frame's fences.
    vkWaitForFences(m_device.get(), 1, &m_inFlightFences[currentFrame].get(), VK_TRUE, UINT64_MAX);

    // If the window is minimized then simply do not render anything anymore.
    if (m_window->minimized())
      return true;

    // If the window size has changed the swapchain has to be recreated.
    if (m_window->changed())
    {
      recreateSwapchain();
      return true;
    }

    uint32_t imageIndex;
    m_swapchain.acquireNextImage(m_imageAvailableSemaphores[currentFrame].get(), VK_NULL_HANDLE, &imageIndex);

    // TODO: Temporary
    for (std::shared_ptr<Model> model : m_models)
    {
      model->m_uniformBuffers.upload(imageIndex, model->getUbo());
    }

    // Check if a previous frame is using the current image.
    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
      vkWaitForFences(m_device.get(), 1, &m_imagesInFlight[currentFrame], VK_TRUE, UINT64_MAX);

    // This will mark the current image to be in use by this frame.
    m_imagesInFlight[imageIndex] = m_inFlightFences[currentFrame].get();

    VkSubmitInfo submitInfo{ };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[currentFrame].get() };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_swapchainCmdBuffers.get()[imageIndex];

    VkSemaphore signalSemaphores[] = { m_finishedRenderSemaphores[currentFrame].get() };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Reset the signaled state of the current frame's fence to the unsignaled one.
    vkResetFences(m_device.get(), 1, &m_inFlightFences[currentFrame].get());

    // Submits / executes the current image's / framebuffer's command buffer.
    m_queues.submit(submitInfo, m_inFlightFences[currentFrame].get());

    VkPresentInfoKHR presentInfo{ };
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_swapchain.get() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    // Tell the presentation engine that the current image is ready.
    m_queues.present(presentInfo);

    currentFrame = (currentFrame + 1) % maxFramesInFlight;

    return true;
  }
  
  void Api::clearModels()
  {
    m_models.clear();
  }

  void Api::pushModel(const std::shared_ptr<Model> model)
  {
    m_models.push_back(model);
  }

  void Api::setModels(const std::vector<std::shared_ptr<Model>>& models)
  {

  }

  void Api::clean()
  {
    m_device.waitIdle();
  }

  void Api::recreateSwapchain()
  {
    RX_LOG("Recreating swapchain.");

    m_device.waitIdle();

    // Cleaning the existing swapchain.
    m_swapchain.destroyDepthImageView();
    m_swapchain.destroyDepthImage();
    m_swapchain.destroyFramebuffers();
    m_swapchainCmdBuffers.free();
    m_pipeline.destroy();
    m_renderPass.destroy();
    m_swapchain.destroyImageViews();
    m_swapchain.destroy();

    for (std::shared_ptr<Model> model : m_models)
      model->m_uniformBuffers.destroy();

    // Recreating the swapchain.
    m_renderPass.initialize(renderPassInfo);

    // Swapchain
    m_surface.checkSettingSupport(m_physicalDevice.get());
    swapchainInfo.surfaceCapabilities = m_surface.getInfo().capabilities;
    m_swapchain.initialize(swapchainInfo);
    m_swapchain.initImageViews();

    // Shaders
    Shader vs;
    ShaderInfo vertexShaderInfo{ };
    vertexShaderInfo.fullPath = RX_SHADER_PATH "simple3D.vert";
    vertexShaderInfo.device = m_device.get();

    vs.initialize(vertexShaderInfo);

    Shader fs;
    ShaderInfo fragmentShaderInfo{ };
    fragmentShaderInfo.fullPath = RX_SHADER_PATH "simple3D.frag";
    fragmentShaderInfo.device = m_device.get();

    fs.initialize(fragmentShaderInfo);

    // Pipeline
    pipelineInfo.viewport = { 0.0f, 0.0f, static_cast<float>(m_swapchain.getInfo().extent.width), static_cast<float>(m_swapchain.getInfo().extent.height), 0.0f, 1.0f };
    m_pipeline.initialize(pipelineInfo);

    m_swapchain.initDepthImage();
    m_swapchain.initDepthImageView();
    m_swapchain.initFramebuffers();

    for (auto model : m_models)
    {
      uniformBufferInfo.uniformBufferObject = model->getUbo();
      model->m_uniformBuffers.initialize(uniformBufferInfo);

      model->m_descriptorPool.initialize(descriptorPoolInfo);
      model->m_descriptorSets.initialize(descriptorSetInfo);
    }

    m_swapchainCmdBuffers.initialize(commandBufferInfo);
    record();

    RX_LOG("Finished swapchain recreation.");
  }

  void Api::record()
  {
    RX_LOG("Started recording.");

    // Set up render pass begin info
    renderPassBeginInfo.renderArea = { 0, 0, m_swapchain.getInfo().extent.width, m_swapchain.getInfo().extent.height };
    renderPassBeginInfo.clearValues = { { 0.3f, 0.3f, 0.3f, 1.0f }, { 1.0f, 0 } };
    renderPassBeginInfo.commandBuffers = m_swapchainCmdBuffers.get();

    for (Framebuffer& framebuffer : m_swapchain.getInfo().framebuffers)
      renderPassBeginInfo.framebuffers.push_back(framebuffer.get());

    m_renderPass.setBeginInfo(renderPassBeginInfo);

    // Start recording the swapchain framebuffers
    for (size_t imageIndex = 0; imageIndex < m_swapchainCmdBuffers.get().size(); ++imageIndex)
    {
      m_swapchainCmdBuffers.begin(imageIndex);
      m_renderPass.begin(imageIndex);

      vkCmdBindPipeline(m_swapchainCmdBuffers.get()[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get());

      for (std::shared_ptr<Model> model : m_models)
      {
        VkBuffer vertexBuffers[] = { model->m_vertexBuffer.get() };
        VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(m_swapchainCmdBuffers.get()[imageIndex], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(m_swapchainCmdBuffers.get()[imageIndex], model->m_indexBuffer.get(), 0, model->m_indexBuffer.getType());
        vkCmdBindDescriptorSets(m_swapchainCmdBuffers.get()[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.getLayout(), 0, 1, &model->m_descriptorSets.get()[imageIndex], 0, nullptr);
        vkCmdDrawIndexed(m_swapchainCmdBuffers.get()[imageIndex], model->m_indexBuffer.getCount(), 1, 0, 0, 0);
      }

      m_renderPass.end(imageIndex);
      m_swapchainCmdBuffers.end(imageIndex);
    }
  }
}