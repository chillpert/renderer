#include "CommandPool.hpp"

namespace RX
{
	CommandPool::CommandPool() :
		BaseComponent("CommandPool") { }

  CommandPool::~CommandPool()
  {
    //destroy();
  }

	void CommandPool::initialize(VkDevice device, uint32_t queueFamilyIndex)
  {
    VkCommandPoolCreateInfo createInfo{ };
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = queueFamilyIndex;

    VK_ASSERT(vkCreateCommandPool(device, &createInfo, nullptr, &m_commandPool), "Failed to create command pool.");

    initializationCallback();
  }

  void CommandPool::destroy()
  {
    assertDestruction();
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
   }

  void CommandPool::reset(VkCommandPoolResetFlags flags)
  {
    assertInitialized("reset");
    VK_ASSERT(vkResetCommandPool(m_device, m_commandPool, flags), "Failed to reset command pool.");
  }
}