#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "api/image/Image.hpp"

namespace RAYEXEC_NAMESPACE
{
  /// A helper class for creating ready-to-use textures.
  /// @ingroup API
  /// @todo textureCounter is being incremented always, even if it is not a new texture. This class should actually look inside Api::textures or sth to assign the correct index.
  class Texture : public Image
  {
  public:
    RX_API Texture( );

    /// @param path The relative path to the texture file.
    /// @param initialize If true, the texture will be initialized right away without an additional call to init().
    RX_API Texture( std::string_view path, bool initialize = true );

    /// @return Returns the texture's image view.
    inline vk::ImageView getImageView( ) { return imageView.get( ); }

    /// @return Returns the texture's sampler.
    inline vk::Sampler getSampler( ) { return sampler.get( ); }

    /// @return Returns the relative path of the texture file.
    inline const std::string& getPath( ) const { return path; }

    /// Creates the texture.
    /// @param path The relative path to the texture file.
    /// @todo Latest changes to this class might be causing errors once texture are working again.
    void init( std::string_view path );

    uint32_t offset;

  private:
    std::string path; ///< The relative path to the texture file.

    vk::UniqueImageView imageView; ///< The texture's Vulkan image view with a unique handle.
    vk::UniqueSampler sampler;     ///< The texture's Vulkan sampler with a unique handle.

    static uint32_t textureCounter;
  };
} // namespace RAYEXEC_NAMESPACE

namespace std
{
  /// @cond INTERNAL
  template <>
  struct hash<RAYEXEC_NAMESPACE::Texture>
  {
    size_t operator( )( const std::shared_ptr<RAYEXEC_NAMESPACE::Texture> texture ) const { return hash<std::string>( )( texture->getPath( ) ); }
  };
  /// @endcond
} // namespace std

#endif // TEXTURE_HPP