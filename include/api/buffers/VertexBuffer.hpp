#ifndef VERTEX_BUFFER_HPP
#define VERTEX_BUFFER_HPP

#include "api/Vertex.hpp"
#include "api/buffers/Buffer.hpp"

namespace RAYEX_NAMESPACE
{
  /// A specialised buffer for vertex data.
  /// @ingroup API
  class VertexBuffer : public Buffer
  {
  public:
    VertexBuffer( ) = default;

    /// @param vertices The vertex data.
    /// @param initialize If true, the vertex buffer will be initialized right away without an additional call to init().
    VertexBuffer( std::vector<Vertex>& vertices, bool initialize = true );

    /// @return Returns the amount of vertices in the buffer.
    [[nodiscard]] auto getCount( ) const -> uint32_t { return count; }

    /// Creates the buffer, allocates memory for it and fills it with provided data.
    /// @param vertices The vertex data.
    void init( const std::vector<Vertex>& vertices );

  private:
    uint32_t count = 0; ///< The amount of vertices in the buffer.
  };
} // namespace RAYEX_NAMESPACE

#endif // VERTEX_BUFFER_HPP
