#ifndef QUEUES_HPP
#define QUEUES_HPP

#include "pch/stdafx.hpp"

namespace RX
{
  struct QueuesInfo
  {
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
  };

  class Queues
  {
  public:
    // This function should be called right after the physical device was enumerated and the 
    // surface was created. The surface has to be created before the physical device is picked.
    void initialize(QueuesInfo& queuesInfo);

    inline QueuesInfo& getInfo() { return m_info; }

    void retrieveAllHandles(VkDevice device);

    void submit(VkSubmitInfo& submitInfo, VkFence fence);
    void present(VkPresentInfoKHR& presentInfo);

    inline uint32_t getGraphicsFamilyIndex() const { return m_graphicsIndex.value(); }
    inline uint32_t getPresentFamilyIndex() const { return m_presentIndex.value(); }

    inline VkQueue getGraphicsQueue() { return m_graphicsQueue; }
    inline VkQueue getPresentQueue() { return m_presentQueue; }

    // Returns a vector filled with the actual unique queue family indices.    
    std::vector<uint32_t> getQueueFamilyIndices();

    // This function can be used at the time a physical device is picked.
    static bool isComplete(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

  private:
    static std::vector<std::optional<uint32_t>> findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    std::optional<uint32_t> m_graphicsIndex;
    std::optional<uint32_t> m_presentIndex;

    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    QueuesInfo m_info;
  };
}

#endif // QUEUES_HPP