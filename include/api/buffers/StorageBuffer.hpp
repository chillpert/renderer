#ifndef STORAGE_BUFFER_HPP
#define STORAGE_BUFFER_HPP

#include "api/buffers/Buffer.hpp"

namespace RENDERER_NAMESPACE
{
  class StorageBuffer : public Buffer
  {
  public:
    StorageBuffer( ) = default;

    template <typename T>
    StorageBuffer( std::vector<T>& data, bool initialize = true )
    {
      if ( initialize, commandBuffer )
        init<T>( data );
    }

    template <typename T>
    void init( std::vector<T>& data )
    {
      vk::DeviceSize size = sizeof( data[0] ) * data.size( );

      // Set up the staging buffer.
      Buffer stagingBuffer( size,                                                                                   // size
                            vk::BufferUsageFlagBits::eTransferSrc,                                                  // usage
                            { g_transferFamilyIndex },                                                              // queueFamilyIndices
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent ); // memoryPropertyFlags

      stagingBuffer.fill<T>( data.data( ) );

      Buffer::init( size,                                                                                  // size                                                 // size
                    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,       // usage
                    { g_transferFamilyIndex },                                                             // queueFamilyIndices                                                    // queueFamilyIndices
                    vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible ); // memoryPropertyFlags                                                                                                                // pNext of memory

      // Copy staging buffer to the actual index buffer.
      stagingBuffer.copyToBuffer( this->buffer.get( ) );
    }
  };
}

#endif // STORAGE_BUFFER_HPP