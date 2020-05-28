#include "Semaphore.hpp"

namespace RX
{
  Semaphore::~Semaphore()
  {
    destroy();
  }

  void Semaphore::initialize(VkDevice device)
  {
    m_device = device;

    VkSemaphoreCreateInfo createInfo{ };
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_CREATE(vkCreateSemaphore(device, &createInfo, nullptr, &m_semaphore), "semaphore");
  }

  void Semaphore::destroy()
  {
    VK_DESTROY(vkDestroySemaphore(m_device, m_semaphore, nullptr), "semaphore");
  }
}