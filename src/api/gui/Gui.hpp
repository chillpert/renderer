#ifndef GUI_HPP
#define GUI_HPP

#include "Surface.hpp"
#include "Queues.hpp"
#include "RenderPass.hpp"
#include "CommandPool.hpp"

namespace RX
{
  class Gui
  {
  public:
    Gui() = delete;
    ~Gui();

    void initialize(
      VkInstance instance, 
      VkPhysicalDevice physicalDevice, 
      VkDevice device, 
      Queues& queues, 
      VkDescriptorPool descriptorPool, 
      Surface& surface, 
      VkRenderPass renderPass,
      SDL_Window* window,
      uint32_t imageCount
    );

    void beginRender();
    void render();
    void endRender(VkDevice device, Queues& queues, VkRenderPass renderPass, VkFramebuffer framebuffer);

    void renderDemo();

    void clean();
    void recreate();

  private:
    ImGui_ImplVulkanH_Window m_mainWindowData;
    ImDrawData* m_drawData;
    SDL_Window* m_window;
    CommandPool m_commandPool;
  };
}

#endif // GUI_HPP