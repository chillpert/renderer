#ifndef SURFACE_HPP
#define SURFACE_HPP

#include "window/Window.hpp"

namespace RX
{
  struct SurfaceInfo
  {
    Window* window;
    vk::Instance instance;
    vk::Format format = vk::Format::eB8G8R8A8Unorm;
    vk::ColorSpaceKHR colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eMailbox;
  };

  class Surface
  {
  public:
    ~Surface();

    inline vk::SurfaceKHR get() { return m_surface; }
    inline SurfaceInfo& getInfo() { return m_info; }
    
    inline vk::SurfaceCapabilitiesKHR getCapabilities() { return m_capabilities; }

    void initialize(SurfaceInfo& info);
    void destroy();

    // Checks if the preferred settings for format, color space and present mode are available.
    // If not, the function will set them to some fall back values.
    // Must be called right after the enumeration of the physical device.
    void checkSettingSupport(vk::PhysicalDevice physicalDevice);

  private:
    vk::SurfaceKHR m_surface;
    SurfaceInfo m_info;

    vk::SurfaceCapabilitiesKHR m_capabilities;
  };
}

#endif // SURFACE_HPP