#include "window/WindowProperties.hpp"

namespace rx
{
  WindowProperties::WindowProperties(int width, int height, const char* title, uint32_t flags) : 
    m_width(width), 
    m_height(height),
    m_title(title),
    m_engineTitle("No engine"),
    m_flags(flags)
  {
    m_flags |= SDL_WINDOW_VULKAN;
  }

  void WindowProperties::resize(int width, int height)
  {
    m_width = width;
    m_height = height;
  }

  void WindowProperties::setTitle(const char* title)
  {
    m_title = title;
  }

  void WindowProperties::setEngineTitle(const char* title)
  {
    m_engineTitle = title;
  }
}