#include "Swapchain.hpp"

namespace RX
{
  void Swapchain::create(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, std::shared_ptr<Window> window, uint32_t* familyIndex)
  {
    checkFormatSupport(physicalDevice, surface);
    checkPhysicalDeviceSurfaceSupport(physicalDevice, surface, familyIndex);
    auto surfaceCapabilities = getSurfaceCapabilitites(physicalDevice, surface);

    int width, height;
    window->getWindowSize(&width, &height);

    VkSwapchainCreateInfoKHR createInfo{ };
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = 2;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.imageExtent.width = static_cast<uint32_t>(width);
    createInfo.imageExtent.height = static_cast<uint32_t>(height);
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = familyIndex;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    
    VK_ASSERT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain), "Failed to create swapchain");
  }

  void Swapchain::destroy(VkDevice device)
  {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
  }

  void Swapchain::checkPhysicalDeviceSurfaceSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* familyIndex)
  {
    VkBool32 supported = false;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, *familyIndex, surface, &supported), "Failed to query pyhsical device surface support");

    if (supported == VK_FALSE)
      VK_ERROR("Physical device surface does not support WSI");
  }

  void Swapchain::checkFormatSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
  {
    // define prefered surface formats
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
    
    VkFormatProperties formatProperties{ };
    vkGetPhysicalDeviceFormatProperties(physicalDevice, surfaceFormat.format, &formatProperties); // TODO: check if this color format is supported

    uint32_t surfaceFormatCounts;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCounts, nullptr), "Failed to query physical device surface formats");

    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCounts);
    VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCounts, surfaceFormats.data()), "Failed to query physical device surface formats");

    for (const auto& iter : surfaceFormats)
    {
      if (iter.format == surfaceFormat.format && iter.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        return;
    }

    // If the wanted format and space are not available, fall back.
    surfaceFormat.format = surfaceFormats[0].format;
    surfaceFormat.colorSpace = surfaceFormats[0].colorSpace;
  }

  VkSurfaceCapabilitiesKHR Swapchain::getSurfaceCapabilitites(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
  {
    VkSurfaceCapabilitiesKHR surfaceCapabilitites;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilitites);

    if (!(surfaceCapabilitites.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR))
      VK_ERROR("VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR is not supported on this device"); // TODO: make generic

    if (!(surfaceCapabilitites.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
      VK_ERROR("VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT is not supported on this device"); // TODO: make generic

    return surfaceCapabilitites;
  }
}