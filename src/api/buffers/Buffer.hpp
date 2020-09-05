#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "Image.hpp"
#include "Components.hpp"

namespace rx
{
  /// A wrapper class for a Vulkan buffer.
  /// @ingroup API
  class Buffer
  {
  public:
    Buffer( ) = default;
    
    /// @param size The size of the buffer.
    /// @param usage The buffer's usage flags.
    /// @param queueFamilyIndices Specifies which queue family will access the buffer.
    /// @param memoryPropertyFlags Flags for memory allocation.
    /// @param pNextMemory Attachment to the memory's pNext chain.
    /// @param initialize If true, the buffer object will be initialized right away without an additional call to init().
    Buffer( vk::DeviceSize size, vk::BufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices = { }, vk::MemoryPropertyFlags memoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal, void* pNextMemory = nullptr, bool initialize = true );

    /// Copies the content of this buffer to another buffer.
    /// @param buffer The target for the copy operation.
    void copyToBuffer( Buffer& buffer ) const;

    /// Copies the content of this buffer to an image.
    /// @param image The target for the copy operation.
    void copyToImage( Image& image ) const;

    /// @return Returns the buffer without the unique handle.
    inline const vk::Buffer get( ) const { return m_buffer.get( ); }

    /// @return Returns the buffer's memory without the unique handle.
    inline const vk::DeviceMemory getMemory( ) const { return m_memory.get( ); }

    /// @return Returns the size of the buffer.
    inline const vk::DeviceSize getSize( ) const { return m_size; }

    /// Creates the buffer and allocates memory for it.
    /// @param size The size of the buffer.
    /// @param usage The buffer's usage flags.
    /// @param queueFamilyIndices Specifies which queue family will access the buffer.
    /// @param memoryPropertyFlags Flags for memory allocation.
    /// @param pNextMemory Attachment to the memory's pNext chain.
    void init( vk::DeviceSize size, vk::BufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices = { }, vk::MemoryPropertyFlags memoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal, void* pNextMemory = nullptr );

    /// Used to fill the buffer with any kind of data.
    /// @param source The data to fill the buffer with.
    /// @param offset The data's offset within the buffer.
    template <class T>
    void fill( T* source, vk::DeviceSize offset = 0 )
    {
      void* data;
      if ( g_device.mapMemory( m_memory.get( ), offset, m_size, { }, &data ) != vk::Result::eSuccess )
        RX_ERROR( "Failed to map memory." );

      memcpy( data, source, static_cast<uint32_t>( m_size ) );

      g_device.unmapMemory( m_memory.get( ) );
    }

  private:
    vk::UniqueBuffer m_buffer; ///< The buffer object with a unique handle.
    vk::UniqueDeviceMemory m_memory; ///< The buffer's memory with a unique handle.

    vk::DeviceSize m_size = 0; ///< The buffer's size.
  };
}

#endif // BUFFER_HPP