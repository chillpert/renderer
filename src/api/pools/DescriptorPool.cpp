#include "DescriptorPool.hpp"

namespace RX
{
  DescriptorPool::DescriptorPool(DescriptorPoolInfo& info)
  {
    init(info);
  }

  DescriptorPool::DescriptorPool(DescriptorPoolInfo&& info)
  {
    init(info);
  }

  DescriptorPool::~DescriptorPool()
  {
    if (m_pool)
      destroy();
  }

  void DescriptorPool::init(DescriptorPoolInfo& info)
  {
    m_info = info;

    vk::DescriptorPoolCreateInfo createInfo;
    createInfo.poolSizeCount = static_cast<uint32_t>(m_info.poolSizes.size());
    createInfo.pPoolSizes = m_info.poolSizes.data();
    createInfo.maxSets = m_info.maxSets;

    m_pool = m_info.device.createDescriptorPool(createInfo);
    if (!m_pool)
      RX_ERROR("Failed to create descriptor pool.");
  }

  void DescriptorPool::init(DescriptorPoolInfo&& info)
  {
    init(info);
  }

  void DescriptorPool::destroy()
  {
    if (!m_pool)
      return;

    m_info.device.destroyDescriptorPool(m_pool);
    m_pool = nullptr;
  }
}