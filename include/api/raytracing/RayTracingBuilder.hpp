#ifndef RAYTRACING_BUILDER_HPP
#define RAYTRACING_BUILDER_HPP

#include "api/raytracing/AccelerationStructure.hpp"
#include "base/Model.hpp"
#include "api/buffers/Buffer.hpp"
#include "api/Swapchain.hpp"
#include "base/GeometryNode.hpp"

namespace RENDERER_NAMESPACE
{
  /// Manages the building process of the acceleration structures.
  /// @ingroup API
  class RayTracingBuilder
  {
  public:
    /// Calls destroy().
    ~RayTracingBuilder( );

    /// Retrieves the physical device's ray tracing capabilities.
    void init( );
    
    /// Destroys all bottom and top level acceleration structures.
    void destroy( );

    /// @return Returns the top level acceleration structure.
    inline const Tlas& getTlas( ) const { return tlas; }

    /// @return Returns the storage image's image view.
    inline const vk::ImageView getStorageImageView( ) const { return storageImageView.get( ); }

    /// Used to convert wavefront models to a bottom level acceleration structure.
    /// @param model A pointer to a rx::Model object.
    /// @return Returns the bottom level acceleration structure.
    Blas modelToBlas( const std::shared_ptr<Model> model ) const;

    /// Used to convert a bottom level acceleration structure instance to a Vulkan geometry instance.
    /// @param instance A bottom level acceleration structure instance.
    /// @return Returns the Vulkan geometry instance.
    vk::AccelerationStructureInstanceKHR instanceToVkGeometryInstanceKHR( const BlasInstance& instance );

    /// Used to prepare building the bottom level acceleration structures.
    /// @param models A vector of pointers to rx::Model objects.
    void createBottomLevelAS( const std::vector<std::shared_ptr<Model>>& models );

    /// Builds all bottom level acceleration structures.
    /// @param blas_ A vector of rx::Blas objects containing all bottom level acceleration structures prepared in createBottomLevelAS().
    /// @param flags The build flags.
    void buildBlas( const std::vector<Blas>& blas_, vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace );

    /// Used to prepare building the top level acceleration structure.
    /// @param nodes A vector of pointers to rx::GeometryNode objects.
    void createTopLevelAS( const std::vector<std::shared_ptr<GeometryNode>>& nodes );

    /// Build the top level acceleration structure.
    /// @param instances A vector of bottom level acceleration structure instances.
    /// @flags The build flags.
    void buildTlas( const std::vector<BlasInstance>& instances, vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace );

    /// Creates the storage image which the ray tracing shaders will write to.
    /// @param swapchainExtent The swapchain images' extent.
    void createStorageImage( vk::Extent2D swapchainExtent );

    /// Creates the shader binding tables.
    /// @param rtPipeline The ray tracing graphics pipeline.
    void createShaderBindingTable( vk::Pipeline rtPipeline );

    /// Used to record the actual ray tracing commands to a given command buffer.
    /// @param swapchainCommandBuffer The command buffer to record to.
    /// @param swapchainImage The current image in the swapchain.
    /// @param extent The swapchain images' extent.
    void rayTrace( vk::CommandBuffer swapchaincommandBuffer, vk::Image swapchainImage, vk::Extent2D extent );

  private:
    vk::PhysicalDeviceRayTracingPropertiesKHR rtProperties; ///< The physical device's ray tracing capabilities.

    std::vector<Blas> blas_; ///< A vector containing all bottom level acceleration structures.
    Tlas tlas; ///< The top level acceleration structure.

    Buffer instanceBuffer; ///< The instance buffer.
  
    Buffer sbtBuffer; ///< The shader binding table buffer.

    Image storageImage; ///< The storage image.
    vk::UniqueImageView storageImageView; ///< The storage image's image view.
  };
}

#endif // RAYTRACING_BUILDER_HPP