#include "api/descriptors/DescriptorSet.hpp"
#include "api/misc/Components.hpp"

namespace RENDERER_NAMESPACE
{
  DescriptorSet::DescriptorSet( vk::DescriptorPool descriptorPool, uint32_t count, const std::vector<vk::DescriptorSetLayout>& layouts, bool initialize )
  {
    if ( initialize )
      init( descriptorPool, count, layouts );
  }

  void DescriptorSet::init( vk::DescriptorPool descriptorPool, uint32_t count, const std::vector<vk::DescriptorSetLayout>& layouts )
  {
    this->descriptorPool = descriptorPool;
    this->layouts = layouts;

    vk::DescriptorSetAllocateInfo allocInfo( descriptorPool,
                                             count,
                                             layouts.data() );

    this->sets = g_device.allocateDescriptorSets(allocInfo);

    for ( const vk::DescriptorSet& set : this->sets )
      RX_ASSERT( set, "Failed to create descriptor sets." );
  }

  void DescriptorSet::update( const vk::AccelerationStructureKHR& tlas, vk::ImageView storageImageView, const std::vector<vk::Buffer>& uniformBuffers )
  {
    for ( size_t i = 0; i < this->layouts.size( ); ++i )
    {
      vk::WriteDescriptorSetAccelerationStructureKHR descriptorInfoAS( 1,       // accelerationStructureCount
                                                                       &tlas ); // pAccelerationStructures 

      vk::DescriptorImageInfo imageInfo( { },                         // sampler
                                         storageImageView,            // imageView
                                         vk::ImageLayout::eGeneral ); // imageLayout

      vk::DescriptorBufferInfo cameraBufferInfo( uniformBuffers[i],      // buffer
                                                 0,                      // offset
                                                 sizeof( CameraUbo ) );  // range

      std::array<vk::WriteDescriptorSet, 3> descriptorWrites;
      descriptorWrites[0] = writeAccelerationStructure( this->sets[i], 0, &descriptorInfoAS );
      descriptorWrites[1] = writeStorageImage( this->sets[i], 1, imageInfo );
      descriptorWrites[2] = writeUniformBuffer( this->sets[i], 2, cameraBufferInfo );

      g_device.updateDescriptorSets( descriptorWrites, 0 );
    }
  }

  void DescriptorSet::update( vk::ImageView textureImageView, vk::Sampler textureSampler, vk::Buffer vertexBuffer, vk::Buffer indexBuffer )
  {
    for ( size_t i = 0; i < this->layouts.size( ); ++i )
    {
      vk::DescriptorImageInfo textureInfo( textureSampler,                            // sampler
                                           textureImageView,                          // imageView
                                           vk::ImageLayout::eShaderReadOnlyOptimal ); // imageLayout

      vk::DescriptorBufferInfo vertbufferInfo( vertexBuffer,             // buffer
                                               0,                        // offset
                                               sizeof( vertexBuffer ) ); // range

      vk::DescriptorBufferInfo indexbufferInfo( indexBuffer,              // buffer
                                                0,                        // offset
                                                sizeof( indexBuffer ) );  // range

      std::array<vk::WriteDescriptorSet, 3> descriptorWrites;
      descriptorWrites[0] = writeCombinedImageSampler( this->sets[i], 0, textureInfo );
      descriptorWrites[1] = writeStorageBuffer( this->sets[i], 1, vertbufferInfo );
      descriptorWrites[2] = writeStorageBuffer( this->sets[i], 2, indexbufferInfo );

      g_device.updateDescriptorSets( descriptorWrites, 0 );
    }
  }

  void DescriptorSet::update( vk::Buffer vertexBuffer, vk::Buffer indexBuffer )
  {
    for ( size_t i = 0; i < this->layouts.size( ); ++i )
    {
      vk::DescriptorBufferInfo vertexBufferInfo( vertexBuffer,             // buffer
                                                 0,                        // offset
                                                 sizeof( vertexBuffer ) ); // range

      vk::DescriptorBufferInfo indexBufferInfo( indexBuffer,             // buffer
                                                0,                       // offset
                                                sizeof( indexBuffer ) ); // range

      std::array<vk::WriteDescriptorSet, 2> descriptorWrites;
      descriptorWrites[0] = writeStorageBuffer( this->sets[i], 0, vertexBufferInfo );
      descriptorWrites[1] = writeStorageBuffer( this->sets[i], 1, indexBufferInfo );

      g_device.updateDescriptorSets( descriptorWrites, 0 );
    }
  }

  void DescriptorSet::update( const std::vector<vk::Buffer>& lightSourcesBuffer, vk::Buffer sceneDescriptionBuffer )
  {
    for ( size_t i = 0; i < this->layouts.size( ); ++i )
    {
      vk::DescriptorBufferInfo lightSourcesBufferInfo( lightSourcesBuffer[i], // buffer
                                                       0,                     // offset
                                                       sizeof( LightsUbo ) ); // range

      vk::DescriptorBufferInfo sceneDescriptionBufferInfo( sceneDescriptionBuffer,             // buffer
                                                           0,                                  // offset
                                                           sizeof( sceneDescriptionBuffer ) ); // range

      if ( sizeof( sceneDescriptionBuffer ) == 8 )
        sceneDescriptionBufferInfo.range = VK_WHOLE_SIZE;

      std::array<vk::WriteDescriptorSet, 2> descriptorWrites;
      descriptorWrites[0] = writeUniformBuffer( this->sets[i], 0, lightSourcesBufferInfo );
      descriptorWrites[1] = writeStorageBuffer( this->sets[i], 1, sceneDescriptionBufferInfo );

      g_device.updateDescriptorSets( descriptorWrites, 0 );
    }
  }

  void DescriptorSet::free( )
  {
    g_device.freeDescriptorSets( this->descriptorPool, this->sets );
  }

  vk::WriteDescriptorSet DescriptorSet::writeUniformBuffer( vk::DescriptorSet descriptorSet, uint32_t binding, const vk::DescriptorBufferInfo& bufferInfo )
  {
    vk::WriteDescriptorSet result( descriptorSet,                      // dstSet
                                   binding,                            // dstBinding
                                   0,                                  // dstArrayElement
                                   1,                                  // descriptorCount
                                   vk::DescriptorType::eUniformBuffer, // descriptorType
                                   nullptr,                            // pImageInfo
                                   &bufferInfo,                        // pBufferInfo
                                   nullptr );                          // pTextelBufferView

    return result;
  }

  vk::WriteDescriptorSet DescriptorSet::writeStorageBuffer( vk::DescriptorSet descriptorSet, uint32_t binding, const vk::DescriptorBufferInfo& bufferInfo )
  {
    vk::WriteDescriptorSet result( descriptorSet,                      // dstSet
                                   binding,                            // dstBinding
                                   0,                                  // dstArrayElement
                                   1,                                  // descriptorCount
                                   vk::DescriptorType::eStorageBuffer, // descriptorType
                                   nullptr,                            // pImageInfo
                                   &bufferInfo,                        // pBufferInfo
                                   nullptr );                          // pTextelBufferView

    return result;
  }

  vk::WriteDescriptorSet DescriptorSet::writeStorageImage( vk::DescriptorSet descriptorSet, uint32_t binding, const vk::DescriptorImageInfo& imageInfo )
  {
    vk::WriteDescriptorSet result( descriptorSet,                     // dstSet
                                   binding,                           // dstBinding
                                   0,                                 // dstArrayElement
                                   1,                                 // descriptorCount
                                   vk::DescriptorType::eStorageImage, // descriptorType
                                   &imageInfo,                        // pImageInfo
                                   nullptr,                           // pBufferInfo
                                   nullptr );                         // pTextelBufferView

    return result;
  }

  vk::WriteDescriptorSet DescriptorSet::writeCombinedImageSampler( vk::DescriptorSet descriptorSet, uint32_t binding, const vk::DescriptorImageInfo& imageInfo )
  {
    vk::WriteDescriptorSet result( descriptorSet,                             // dstSet
                                   binding,                                   // dstBinding
                                   0,                                         // dstArrayElement
                                   1,                                         // descriptorCount
                                   vk::DescriptorType::eCombinedImageSampler, // descriptorType
                                   &imageInfo,                                // pImageInfo
                                   nullptr,                                   // pBufferInfo
                                   nullptr );                                 // pTextelBufferView

    return result;
  }

  vk::WriteDescriptorSet DescriptorSet::writeAccelerationStructure( vk::DescriptorSet descriptorSet, uint32_t binding, void* pNext )
  {
    vk::WriteDescriptorSet result( descriptorSet,                                 // dstSet
                                   binding,                                       // dstBinding
                                   0,                                             // dstArrayElement
                                   1,                                             // descriptorCount
                                   vk::DescriptorType::eAccelerationStructureKHR, // descriptorType
                                   nullptr,                                       // pImageInfo
                                   nullptr,                                       // pBufferInfo
                                   nullptr );                                     // pTextelBufferView
    result.pNext = pNext;

    return result;
  }
}