#ifndef UTIL_HPP
#define UTIL_HPP

#include "pch/stdafx.hpp"

namespace RAYEXEC_NAMESPACE
{
  namespace Util
  {
    /// Parses a given shader file.
    /// @param path The full path to shader file.
    /// @return Returns a vector of chars that contains the shader in SPIR-V format.
    auto parseShader( std::string_view path ) -> std::vector<char>;

    void processShaderMacros( std::string_view path, uint32_t dirLightNodes, uint32_t pointLightNodes, uint32_t totalModels );

    /// Used to find any given element inside a STL container.
    /// @param value The value to search for.
    /// @param values The STL container of the same type as value.
    /// @return Returns true, if value was found in values.
    template <typename T, typename Container>
    auto find( T value, const Container& values ) -> bool
    {
      for ( const auto& it : values )
      {
        if ( it == value )
          return true;
      }

      return false;
    }

    auto vec4toArray( const glm::vec4& vec ) -> std::array<float, 4>;
  } // namespace Util
} // namespace RAYEXEC_NAMESPACE

#endif // UTIL_HPP