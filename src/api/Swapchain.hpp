#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include "BaseComponent.hpp"
#include "Surface.hpp"
#include "QueueManager.hpp"

namespace RX
{
  class Swapchain : public BaseComponent
  {
  public:
    Swapchain();

    inline VkSwapchainKHR get() { return swapchain; }
    
    inline std::vector<VkImage>& getImages() { return images; }
    inline std::vector<VkImageView>& getImageViews() { return imageViews; }
    inline std::vector<VkFramebuffer> getFramebuffers() { return framebuffers; }
    
    inline VkExtent2D& getExtent() { return extent; } 

    void initialize(VkPhysicalDevice physicalDevice, VkDevice device, Surface surface, std::shared_ptr<Window> window, QueueManager& queueManager);
    void destroy(VkDevice device);
    
    void initializeImages(VkDevice device);
    void initializeImageViews(VkDevice device, Surface surface);
    void initializeFramebuffers(VkDevice device, VkRenderPass renderPass, std::shared_ptr<Window> window);

  private:
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkExtent2D extent;
  };
}

#endif // SWAPCHAIN_HPP