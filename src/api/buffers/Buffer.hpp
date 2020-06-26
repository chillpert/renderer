#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "InitializerInfos.hpp"
#include "Image.hpp"

namespace RX
{
  struct BufferInfo
  {
    // General information
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    vk::CommandPool commandPool; // Optional, if there is no staging or copying involved.
    vk::Queue queue; // Optional, if there is no staging or copying involved.

    // Buffer
    void* pNextBuffer = nullptr; // Optional
    vk::BufferCreateFlags bufferFlags; // Optional
    vk::DeviceSize size; // The size required for the buffer.
    vk::BufferUsageFlags usage;
    vk::SharingMode sharingMode;
    std::vector<uint32_t> queueFamilyIndices; // Optional, if sharing mode is not concurrent.

    // Memory
    vk::MemoryPropertyFlags memoryProperties;
    void* pNextMemory = nullptr; // Optional
    vk::DeviceSize memoryOffset = 0;
  };

  class Buffer
  {
  public:
    Buffer() = default;
    Buffer(BufferInfo& createInfo);
    Buffer(BufferInfo&& createInfo);
    RX_API ~Buffer();

    Buffer& operator=(const Buffer& buffer);
    void copyToBuffer(const Buffer& buffer) const;
    void copyToImage(Image& image) const;

    inline vk::Buffer get() const { return m_buffer; }
    inline vk::DeviceMemory& getMemory() { return m_memory; }
    inline vk::DeviceSize getSize() const { return m_info.size; }

    void init(BufferInfo& createInfo);
    void init(BufferInfo&& createInfo);

    template <class T>
    void fill(T* source);

    RX_API void destroy();

  private:
    vk::Buffer m_buffer;
    vk::DeviceMemory m_memory;
    BufferInfo m_info;
  };

  template <class T>
  inline void Buffer::fill(T* source)
  {
    void* data;
    vkMapMemory(m_info.device, m_memory, 0, m_info.size, 0, &data);
    memcpy(data, source, static_cast<uint32_t>(m_info.size));
    vkUnmapMemory(m_info.device, m_memory);
  }
}

#endif // BUFFER_HPP