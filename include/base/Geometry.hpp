#pragma once

#include "api/Vertex.hpp"

namespace RAYEX_NAMESPACE
{
  /// Describes the rendering properties of a mesh.
  ///
  /// Property descriptions copied from https://www.loc.gov/preservation/digital/formats/fdd/fdd000508.shtml.
  /// @ingroup BASE
  struct Material
  {
    glm::vec3 ka = glm::vec3( 0.0F, 0.0F, 0.0F ); /// Ambient color
    glm::vec3 kd = glm::vec3( 0.0F, 0.0F, 0.0F ); /// Diffuse color
    glm::vec3 ks = glm::vec3( 0.0F, 0.0F, 0.0F ); /// Specular color

    std::string ambientTexPath  = "";
    std::string diffuseTexPath  = "";
    std::string specularTexPath = "";

    glm::vec3 emission = glm::vec3( 0.0F );

    /// Illumination model.
    /// 0 - A constant color illumination model, using the Kd for the material.
    /// 1 - A diffuse illumination model using Lambertian shading, taking into account Ka, Kd, the intensity and position of each light source and the angle at which it strikes the surface.
    /// 2 - A diffuse and specular illumination model using Lambertian shading and Blinn's interpretation of Phong's specular illumination model, taking into account Ka, Kd, Ks, and the intensity and position of each light source and the angle at which it strikes the surface.
    uint32_t illum = 0;

    /// Specifies a factor for dissolve, how much this material dissolves into the background. A factor of 1.0 is fully opaque. A factor of 0.0 is completely transparent.
    float d = 1.0F;

    /// Focus of the specular light (aka shininess). Ranges from 0 to 1000, with a high value resulting in a tight, concentrated highlight.
    float ns = 0.0F;

    /// Optical density (aka index of refraction). Ranges from 0.001 to 10. A value of 1.0 means that light does not bend as it passes through an object.
    float ni = 1.0F;
  };

  /// Describes a sub-mesh and its material.
  /// @warning If indexOffset is not set correctly the mesh can not be displayed properly. Take a look at loadObj(std::string_view) for a working example.
  /// @ingroup BASE
  struct Mesh
  {
    Material material    = { }; ///< The mesh's material.
    uint32_t indexOffset = 0;   ///< Refers to the offset of this mesh inside a Geometry::indices container.
  };

  /// Describes a geometry Rayex can render.
  ///
  /// Even if a model consists out of multiple sub-meshes, all vertices and indices must be stored together in their respective containers.
  /// @warning geometryIndex must be incremented everytime a new model is created.
  /// @ingroup BASE
  struct Geometry
  {
    RX_API void setMaterial( const Material& material );

    std::vector<Vertex> vertices;   ///< Contains all vertices of the geometry.
    std::vector<uint32_t> indices;  ///< Contains all indices of the geometry.
    std::vector<Mesh> meshes;       ///< Contains all sub-meshes and their respective materials.
    uint32_t geometryIndex = 0;     ///< A unique index required by the acceleration structures.
    std::string path       = "";    ///< The model's path, relative to the path to assets.
    bool initialized       = false; ///< Keeps track of whether or not the geometry was initialized.
    bool dynamic           = false; ///< Keeps track of whether or not the geometry is dynamic or static.
  };

  /// Describes an instance of some geometry.
  /// @warning To assign a specific geometry to an instance, both must have the same value for geometryIndex.
  /// @ingroup BASE
  struct GeometryInstance
  {
    RX_API void setTransform( const glm::mat4& transform );

    glm::mat4 transform    = glm::mat4( 1.0F ); ///< The instance's world transform matrix.
    glm::mat4 transformIT  = glm::mat4( 1.0F ); ///< The inverse transpose of transform.
    uint32_t geometryIndex = 0;                 ///< Used to assign this instance a model.
  };

  /// A commodity function for loading a wavefront (.obj) model file and allocate a geometry object from it.
  ///
  /// The function will attempt to find sub-meshes in the file and retrieve all materials.
  /// A user is encouraged to create their own model loader function or classes.
  /// @param path The model's path, relative to the path to assets.
  /// @param dynamic If true, the geometry can be animated. Otherwise the geometry is static throughout its entire lifetime.
  /// @return Returns a pointer to a geometry object.
  /// @ingroup BASE
  RX_API std::shared_ptr<Geometry> loadObj( std::string_view path, bool dynamic = false );

  /// A commodity function for allocating an instance from a given geometry and set its matrices.
  ///
  /// The function will also automatically set the inverse transpose matrix.
  /// @param geometry The geometry to create the instance from.
  /// @param transform The world transform matrix of the instance.
  /// @return Returns a pointer to a geometry instance.
  /// @ingroup BASE
  RX_API std::shared_ptr<GeometryInstance> instance( std::shared_ptr<Geometry> geometry, const glm::mat4& transform = glm::mat4( 1.0F ) );

} // namespace RAYEX_NAMESPACE
