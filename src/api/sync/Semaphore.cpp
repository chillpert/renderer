#include "Semaphore.hpp"

namespace RX
{
  Semaphore::Semaphore() :
    BaseComponent("Semaphore") { }

  Semaphore::~Semaphore()
  {
    destroy();
  }

  void Semaphore::initialize(VkDevice device)
  {
    m_device = device;

    VkSemaphoreCreateInfo createInfo{ };
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_ASSERT(vkCreateSemaphore(device, &createInfo, nullptr, &m_semaphore), "Failed to create semaphore.");

    initializationCallback();
  }

  void Semaphore::destroy()
  {
    assertDestruction();
    vkDestroySemaphore(m_device, m_semaphore, nullptr);
  }
}