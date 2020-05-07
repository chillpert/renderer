#ifndef COMMAND_BUFFER_HPP
#define COMMAND_BUFFER_HPP

#include "api/vulkan/SwapChain.hpp"
#include "api/vulkan/Pipeline.hpp"

namespace RX
{
  class CommandBuffer
  {
  public:
    CommandBuffer(VkPhysicalDevice* physicalDevice, VkDevice* logicalDevice, SwapChain* swapChain, Pipeline* pipeline);

    void createCommandPool();
    void createCommandBuffers();

    void destroyCommandPool();

  private:
    void startCommandBufferRecording();
    
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;

    VkPhysicalDevice* m_physicalDevice;
    VkDevice* m_logicalDevice;

    // Pointers to VulkanApi class member
    SwapChain* m_swapChain;
    Pipeline* m_pipeline;
  };
}

#endif // COMMAND_BUFFER_HPP