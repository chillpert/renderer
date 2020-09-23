#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "base/Window.hpp"
#include "api/Swapchain.hpp"
#include "Settings.hpp"

namespace RENDERER_NAMESPACE
{
  /// A wrapper class for a Vulkan graphics pipeline.
  /// @ingroup API
  class Pipeline
  {
  public:
    Pipeline( ) = default;

    /// Initializes a rasterization pipeline.
    /// @renderPass A Vulkan render pass.
    /// @descriptorSetLayouts A vector of descriptor set layouts that will be included in the pipeline layout.
    void init( const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts, vk::RenderPass renderPass, vk::Viewport viewport, vk::Rect2D scissor );

    /// Initializes a ray tracing pipeline.
    /// @param descriptorSetLayouts A vector of descriptor set layouts that will be included in the pipeline layout.
    void init( const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts, const Settings* const settings );

    /// @return Returns the Vulkan pipeline object without the unique handle.
    inline const vk::Pipeline get( ) const { return pipeline.get( ); }

    /// @return Returns the Vulkan pipeline layout object without the unique handle.
    inline const vk::PipelineLayout getLayout( ) const { return layout.get( ); }

    /// Binds the pipeline for usage.
    /// @param commandBuffer The Vulkan command buffer that will bind the pipeline.
    void bind( vk::CommandBuffer commandBuffer ) const;

  private:
    vk::UniquePipeline pipeline; ///< The Vulkan pipeline with a unique handle.
    vk::UniquePipelineLayout layout; ///< The Vulkan pipeline layout with a unique handle.
  };
}

#endif // PIPELINE_HPP