#include "VertexBuffer.hpp"

namespace RX
{
  void VertexBuffer::initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, std::vector<Vertex>& vertices)
  {
    // Set up the staging buffer.
    BufferCreateInfo stagingInfo{ };
    stagingInfo.physicalDevice = physicalDevice;
    stagingInfo.device = device;
    stagingInfo.deviceSize = sizeof(vertices[0]) * vertices.size();
    stagingInfo.count = static_cast<uint32_t>(vertices.size());
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    stagingInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingInfo.commandPool = commandPool;
    stagingInfo.queue = queue;

    // Set up the actual index buffer.
    BufferCreateInfo bufferInfo = stagingInfo;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    Buffer stagingBuffer;
    stagingBuffer.create(stagingInfo);
    stagingBuffer.fill<Vertex>(vertices.data());
 
    m_buffer.create(bufferInfo);

    // Copy staging buffer to the actual index buffer.
    m_buffer = stagingBuffer;
  }
}