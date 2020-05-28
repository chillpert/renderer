#ifndef API_HPP
#define API_HPP

#include "Instance.hpp"
#include "DebugMessenger.hpp"
#include "Queues.hpp"
#include "Surface.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "Model.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"
#include "Semaphore.hpp"
#include "Fence.hpp"
#include "Texture.hpp"
#include "Vertex.hpp"
#include "CommandPool.hpp"
#include "CommandBuffer.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "DescriptorSets.hpp"
#include "DescriptorPool.hpp"
#include "DescriptorSetLayout.hpp"
#include "UniformBuffers.hpp"
#include "Gui.hpp"

namespace RX
{
  class Api
  {
  public:
    Api(std::shared_ptr<Window> window);
    RX_API ~Api();

    void initialize();
    bool update();
    bool render();
  
    void clearModels();
    void pushModel(const std::shared_ptr<Model> model);
    void setModels(const std::vector<std::shared_ptr<Model>>& models);

  private:
    void clean();
    void recreateSwapchain();
    void record();

    std::shared_ptr<Window> m_window;

    // Destruction through RAII for following members:
    Instance m_instance;
    DebugMessenger m_debugMessenger;
    Surface m_surface;
    Device m_device;
    RenderPass m_renderPass;
    Swapchain m_swapchain;
    DescriptorSetLayout m_descriptorSetLayout;
    Pipeline m_pipeline;
    CommandPool m_graphicsCmdPool;
    std::vector<Fence> m_inFlightFences;
    std::vector<Semaphore> m_imageAvailableSemaphores;
    std::vector<Semaphore> m_finishedRenderSemaphores;

    CommandBuffer m_swapchainCmdBuffers;
    std::vector<std::shared_ptr<Model>> m_models;

    // No destruction necessary for following members:
    PhysicalDevice m_physicalDevice;
    Queues m_queues;
    std::vector<VkFence> m_imagesInFlight;
  };
}

#endif // API_HPP