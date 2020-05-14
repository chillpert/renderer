#include "QueueManager.hpp"

namespace RX
{
  bool QueueManager::created = false;

  std::optional<uint32_t> QueueManager::graphicsIndex;
  std::optional<uint32_t> QueueManager::presentIndex;

  void QueueManager::create(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
  {
    if (created)
    {
      VK_LOG("Queues have already been evaluated");
      return;
    }

    if (physicalDevice == VK_NULL_HANDLE)
      VK_ERROR("Queue families can not be set up because a physical device has not been picked yet");

    if (surface == VK_NULL_HANDLE)
      VK_ERROR("Queue families can not be set up because the surface has not been created yet");

    auto temp = findQueueFamilies(physicalDevice, surface);
    graphicsIndex = temp.first;
    presentIndex = temp.second; 

    created = true;
  }

  uint32_t QueueManager::getGraphicsIndex()
  {
    if (!created)
      VK_ERROR("Queue families have not been evaluated yet. Make sure to call QueueManager::create first");

    return graphicsIndex.value();
  }

  uint32_t QueueManager::getPresentIndex()
  {
    if (!created)
      VK_ERROR("Queue families have not been evaluated yet. Make sure to call QueueManager::create first");

    return presentIndex.value();
  }

  bool QueueManager::isComplete(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
  {
    auto temp = findQueueFamilies(physicalDevice, surface);
    
    if (temp.first.has_value() && temp.second.has_value())
      return true;

    return false;
  }

  std::pair<std::optional<uint32_t>, std::optional<uint32_t>> QueueManager::findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
  {
    std::optional<uint32_t> graphicsIndex_t;
    std::optional<uint32_t> presentIndex_t;

    uint32_t propertiesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertiesCount, nullptr);

    std::vector<VkQueueFamilyProperties> properties(propertiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertiesCount, properties.data());

    uint32_t index = 0;
    for (const auto& property : properties)
    {
      if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        graphicsIndex_t = index;

      // Check if the current queue index is able to present.
      VkBool32 supported;
      VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &supported), "Failed to get physical device surface support");

      if (supported)
        presentIndex_t = index;

      if (graphicsIndex_t.has_value() && presentIndex_t.has_value())
        break;           

      ++index;
    }

    return { graphicsIndex_t, presentIndex_t };
  }
}