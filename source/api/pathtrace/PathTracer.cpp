#include "api/pathtrace/PathTracer.hpp"

#include "api/Components.hpp"
#include "base/Settings.hpp"

namespace RAYEX_NAMESPACE
{
  PathTracer::~PathTracer( )
  {
    destroy( );
  }

  void PathTracer::init( )
  {
    auto pipelineProperties                       = vkCore::global::physicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR, vk::PhysicalDeviceAccelerationStructurePropertiesKHR>( );
    _capabilities.pipelineProperties              = pipelineProperties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>( );
    _capabilities.accelerationStructureProperties = pipelineProperties.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>( );
  }

  void PathTracer::destroy( )
  {
    vkCore::global::device.waitIdle( );

    for ( Blas& blas : _blas_ )
    {
      blas.as.destroy( );
    }

    _tlas.as.destroy( );

    _blas_.clear( );
  }

  auto PathTracer::modelToBlas( const vkCore::StorageBuffer<Vertex>& vertexBuffer, const vkCore::StorageBuffer<uint32_t>& indexBuffer, bool opaque ) const -> Blas
  {
    // Using index 0, because there are no copies of these buffers.
    vk::BufferDeviceAddressInfo vertexAddressInfo( vertexBuffer.get( 0 ) );
    vk::BufferDeviceAddressInfo indexAddressInfo( indexBuffer.get( 0 ) );

    vk::DeviceAddress vertexAddress = vkCore::global::device.getBufferAddress( vertexAddressInfo );
    vk::DeviceAddress indexAddress  = vkCore::global::device.getBufferAddress( indexAddressInfo );

    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData( Vertex::getVertexPositionFormat( ), // vertexFormat
                                                                     vertexAddress,                      // vertexData
                                                                     sizeof( Vertex ),                   // vertexStride
                                                                     vertexBuffer.getCount( ),           // maxVertex
                                                                     vk::IndexType::eUint32,             // indexType
                                                                     indexAddress,                       // indexData
                                                                     { } );                              // transformData

    vk::AccelerationStructureGeometryKHR asGeom( vk::GeometryTypeKHR::eTriangles,                                                                     // geometryType
                                                 trianglesData,                                                                                       // geometry
                                                 opaque ? vk::GeometryFlagBitsKHR::eOpaque : vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation ); // flags

    vk::AccelerationStructureBuildRangeInfoKHR offset( indexBuffer.getCount( ) / 3, // primitiveCount
                                                       0,                           // primitiveOffset
                                                       0,                           // firstVertex
                                                       0 );                         // transformOffset

    Blas blas;
    blas.asGeometry.push_back( asGeom );
    blas.asBuildRangeInfo.push_back( offset );

    return blas;
  }

  auto PathTracer::geometryInstanceToAccelerationStructureInstance( std::shared_ptr<GeometryInstance>& geometryInstance ) -> vk::AccelerationStructureInstanceKHR
  {
    RX_ASSERT( _blas_.size( ) > geometryInstance->geometryIndex, "Failed to transform geometry instance to a VkGeometryInstanceKHR because index is out of bounds." );
    Blas& blas { _blas_[geometryInstance->geometryIndex] };

    vk::AccelerationStructureDeviceAddressInfoKHR addressInfo( blas.as.as );
    vk::DeviceAddress blasAddress = vkCore::global::device.getAccelerationStructureAddressKHR( addressInfo );

    glm::mat4 transpose = glm::transpose( geometryInstance->transform );

    vk::AccelerationStructureInstanceKHR gInst( { },                                                         // transform
                                                geometryInstance->geometryIndex,                             // instanceCustomIndex
                                                0xFF,                                                        // mask
                                                0,                                                           // instanceShaderBindingTableRecordOffset
                                                vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable, // flags
                                                blasAddress );                                               // accelerationStructureReference

    memcpy( &gInst.transform, &transpose, sizeof( gInst.transform ) );

    return gInst;
  }

  void PathTracer::createBottomLevelAS( std::vector<vkCore::StorageBuffer<Vertex>>& vertexBuffers, const std::vector<vkCore::StorageBuffer<uint32_t>>& indexBuffers, const std::vector<std::shared_ptr<Geometry>>& geometries )
  {
    RX_ASSERT( !vertexBuffers.empty( ), "Failed to build bottom level acceleration structures because no geometry was provided." );

    // Clean up previous acceleration structures and free all memory.
    destroy( );

    _blas_.reserve( vertexBuffers.size( ) );

    for ( size_t i = 0; i < vertexBuffers.size( ); ++i )
    {
      if ( geometries.size( ) > i )
      {
        if ( geometries[i] != nullptr )
        {
          auto blas = modelToBlas( vertexBuffers[i], indexBuffers[i], geometries[i]->isOpaque );
          _blas_.push_back( blas );
        }
      }
    }

    buildBlas( vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace );
  }

  void PathTracer::buildBlas( vk::BuildAccelerationStructureFlagsKHR flags )
  {
    uint32_t blasCount = static_cast<uint32_t>( _blas_.size( ) );

    bool doCompaction = ( flags & vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction ) == vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction;

    vk::DeviceSize maxScratch = 0; // Largest scratch buffer for our BLAS

    std::vector<vk::DeviceSize> originalSizes;
    originalSizes.resize( _blas_.size( ) );

    std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildInfos;
    buildInfos.reserve( _blas_.size( ) );

    // Iterate over the groups of geometries, creating one BLAS for each group
    int index = 0;
    for ( Blas& blas : _blas_ )
    {
      if ( !blas.as.as )
      {
        vkCore::global::device.destroyAccelerationStructureKHR( blas.as.as );
      }

      if ( !blas.as.memory )
      {
        vkCore::global::device.freeMemory( blas.as.memory );
      }

      if ( !blas.as.buffer )
      {
        vkCore::global::device.destroyBuffer( blas.as.buffer );
      }

      vk::AccelerationStructureBuildGeometryInfoKHR buildInfo( vk::AccelerationStructureTypeKHR::eBottomLevel,   // type
                                                               flags,                                            // flags
                                                               vk::BuildAccelerationStructureModeKHR::eBuild,    // mode
                                                               nullptr,                                          // srcAccelerationStructure
                                                               { },                                              // dstAccelerationStructure
                                                               static_cast<uint32_t>( blas.asGeometry.size( ) ), // geometryCount
                                                               blas.asGeometry.data( ),                          // pGeometries
                                                               { },                                              // ppGeometries
                                                               { } );                                            // scratchData

      std::vector<uint32_t> maxPrimitiveCount( blas.asBuildRangeInfo.size( ) );

      for ( size_t i = 0; i < blas.asBuildRangeInfo.size( ); ++i )
      {
        maxPrimitiveCount[i] = blas.asBuildRangeInfo[i].primitiveCount;
      }

      vk::AccelerationStructureBuildSizesInfoKHR sizeInfo;
      vkCore::global::device.getAccelerationStructureBuildSizesKHR( vk::AccelerationStructureBuildTypeKHR::eDevice, &buildInfo, maxPrimitiveCount.data( ), &sizeInfo );

      // Create acceleration structure
      // @todo Potentially, pass size and type to initAccelerationStructure.
      vk::AccelerationStructureCreateInfoKHR createInfo( { },                                            // createFlags
                                                         { },                                            // buffer
                                                         { },                                            // offset
                                                         sizeInfo.accelerationStructureSize,             // size
                                                         vk::AccelerationStructureTypeKHR::eBottomLevel, // type
                                                         { } );                                          // deviceAddress

      blas.as = initAccelerationStructure( createInfo );
      //blas.flags = flags;

      buildInfo.dstAccelerationStructure = blas.as.as;

      maxScratch           = std::max( maxScratch, sizeInfo.buildScratchSize );
      originalSizes[index] = sizeInfo.accelerationStructureSize;

      buildInfos.push_back( buildInfo );

      ++index;
    }

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder.
    vk::MemoryAllocateFlagsInfo allocateFlags( vk::MemoryAllocateFlagBitsKHR::eDeviceAddress );

    vkCore::Buffer scratchBuffer( maxScratch,                                                                              // size
                                  vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, // usage
                                  { vkCore::global::graphicsFamilyIndex },                                                 // queueFamilyIndices
                                  vk::MemoryPropertyFlagBits::eDeviceLocal,                                                // memoryPropertyFlags
                                  &allocateFlags );

    vk::BufferDeviceAddressInfo bufferInfo( scratchBuffer.get( ) );
    vk::DeviceAddress scratchAddress = vkCore::global::device.getBufferAddress( &bufferInfo );

    // Query size of compact BLAS.
    vk::UniqueQueryPool queryPool = vkCore::initQueryPoolUnique( blasCount, vk::QueryType::eAccelerationStructureCompactedSizeKHR );

    // Create a command buffer containing all the BLAS builds.
    vk::UniqueCommandPool commandPool = vkCore::initCommandPoolUnique( { vkCore::global::graphicsFamilyIndex } );
    int ctr                           = 0;

    vkCore::CommandBuffer cmdBuf( commandPool.get( ), blasCount );

    index = 0;
    for ( Blas& blas : _blas_ )
    {
      buildInfos[index].scratchData.deviceAddress = scratchAddress;

      std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos( blas.asBuildRangeInfo.size( ) );

      size_t infoIndex = 0;
      for ( auto& pbuildRangeInfo : pBuildRangeInfos )
      {
        pbuildRangeInfo = &blas.asBuildRangeInfo[infoIndex];
        ++infoIndex;
      }

      cmdBuf.begin( index );

      // Building the acceleration structure
      cmdBuf.get( index ).buildAccelerationStructuresKHR( 1, &buildInfos[index], pBuildRangeInfos.data( ) );

      // Make sure the BLAS were successfully built first before reusing the scratch buffer.
      vk::MemoryBarrier barrier( vk::AccessFlagBits::eAccelerationStructureWriteKHR,  // srcAccessMask
                                 vk::AccessFlagBits::eAccelerationStructureReadKHR ); // dstAccessMask

      cmdBuf.get( index ).pipelineBarrier( vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, // srcStageMask
                                           vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, // dstStageMask
                                           { },                                                       // dependencyFlags
                                           1,                                                         // memoryBarrierCount
                                           &barrier,                                                  // pMemoryBarriers
                                           0,                                                         // bufferMemoryBarrierCount
                                           nullptr,                                                   // pBufferMemoryBarriers
                                           0,                                                         // imageMemoryBarrierCount
                                           nullptr );                                                 // pImageMemoryBarriers

      if ( doCompaction )
      {
        cmdBuf.get( index ).writeAccelerationStructuresPropertiesKHR( 1,                                                     // accelerationStructureCount
                                                                      &blas.as.as,                                           // pAccelerationStructures
                                                                      vk::QueryType::eAccelerationStructureCompactedSizeKHR, // queryType
                                                                      queryPool.get( ),                                      // queryPool
                                                                      ctr++ );                                               // firstQuery
      }

      cmdBuf.end( index );

      ++index;
    }

    cmdBuf.submitToQueue( vkCore::global::graphicsQueue );

    if ( doCompaction )
    {
      vkCore::CommandBuffer compactionCmdBuf( vkCore::global::graphicsCmdPool );

      std::vector<vk::DeviceSize> compactSizes( _blas_.size( ) );

      auto result = vkCore::global::device.getQueryPoolResults( queryPool.get( ),                                // queryPool
                                                                0,                                               // firstQuery
                                                                static_cast<uint32_t>( compactSizes.size( ) ),   // queryCount
                                                                compactSizes.size( ) * sizeof( vk::DeviceSize ), // dataSize
                                                                compactSizes.data( ),                            // pData
                                                                sizeof( vk::DeviceSize ),                        // stride
                                                                vk::QueryResultFlagBits::eWait );                // flags

      RX_ASSERT( result == vk::Result::eSuccess, "Failed to get query pool results." );

      std::vector<AccelerationStructure> cleanupAS( _blas_.size( ) );

      uint32_t totalOriginalSize = 0;
      uint32_t totalCompactSize  = 0;

      compactionCmdBuf.begin( 0 );

      for ( int i = 0; i < _blas_.size( ); ++i )
      {
        totalOriginalSize += static_cast<uint32_t>( originalSizes[i] );
        totalCompactSize += static_cast<uint32_t>( compactSizes[i] );

        // Creating a compact version of the AS.
        vk::AccelerationStructureCreateInfoKHR asCreateInfo( { },                                            // createFlags
                                                             { },                                            // buffer
                                                             { },                                            // offset
                                                             compactSizes[i],                                // size
                                                             vk::AccelerationStructureTypeKHR::eBottomLevel, // type
                                                             { } );                                          // deviceAddress

        auto as = initAccelerationStructure( asCreateInfo );

        // Copy the original BLAS to a compact version
        vk::CopyAccelerationStructureInfoKHR copyInfo( _blas_[i].as.as,                                  // src
                                                       as.as,                                            // dst
                                                       vk::CopyAccelerationStructureModeKHR::eCompact ); // mode

        compactionCmdBuf.get( 0 ).copyAccelerationStructureKHR( &copyInfo );

        cleanupAS[i] = _blas_[i].as;
        _blas_[i].as = as;
      }

      compactionCmdBuf.end( 0 );
      compactionCmdBuf.submitToQueue( vkCore::global::graphicsQueue );

      for ( auto& as : cleanupAS )
      {
        as.destroy( );
      }

      RX_VERBOSE( "BLAS: Compaction Results: ", totalOriginalSize, " -> ", totalCompactSize, " | Total: ", totalOriginalSize - totalCompactSize );
    }
  }

  void PathTracer::updateTlas( const std::vector<std::shared_ptr<GeometryInstance>>& geometryInstances, vk::BuildAccelerationStructureFlagsKHR flags )
  {
    buildTlas( geometryInstances, flags, true );
  }

  void PathTracer::buildTlas( const std::vector<std::shared_ptr<GeometryInstance>>& geometryInstances, vk::BuildAccelerationStructureFlagsKHR flags, bool reuse )
  {
    //_tlas.flags = flags;

    std::vector<vk::AccelerationStructureInstanceKHR> tlasInstances;
    tlasInstances.reserve( geometryInstances.size( ) );

    for ( auto instance : geometryInstances )
    {
      tlasInstances.push_back( geometryInstanceToAccelerationStructureInstance( instance ) );
    }

    if ( reuse )
    {
      // destroy geometry instances buffer (probably not necessary in this case because I am using a unique handle)
    }

    vk::MemoryAllocateFlagsInfo allocateFlags( vk::MemoryAllocateFlagBitsKHR::eDeviceAddress );

    _instanceBuffer.init( sizeof( vk::AccelerationStructureInstanceKHR ) * geometryInstances.size( ),
                          vk::BufferUsageFlagBits::eShaderDeviceAddress,
                          { vkCore::global::graphicsFamilyIndex },
                          vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent,
                          &allocateFlags );

    _instanceBuffer.fill<vk::AccelerationStructureInstanceKHR>( tlasInstances );

    vk::BufferDeviceAddressInfo bufferInfo( _instanceBuffer.get( ) );
    vk::DeviceAddress instanceAddress = vkCore::global::device.getBufferAddress( &bufferInfo );

    vk::UniqueCommandPool commandPool = vkCore::initCommandPoolUnique( vkCore::global::graphicsFamilyIndex );
    vkCore::CommandBuffer cmdBuf( commandPool.get( ) );

    cmdBuf.begin( 0 );

    vk::MemoryBarrier barrier( vk::AccessFlagBits::eTransferWrite,                   // srcAccessMask
                               vk::AccessFlagBits::eAccelerationStructureWriteKHR ); // dstAccessMask

    cmdBuf.get( 0 ).pipelineBarrier( vk::PipelineStageFlagBits::eTransfer,                      // srcStageMask
                                     vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, // dstStageMask
                                     { },                                                       // dependencyFlags
                                     1,                                                         // memoryBarrierCount
                                     &barrier,                                                  // pMemoryBarriers
                                     0,                                                         // bufferMemoryBarrierCount
                                     nullptr,                                                   // pBufferMemoryBarriers
                                     0,                                                         // imageMemoryBarrierCount
                                     nullptr );                                                 // pImageMemoryBarriers

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData( VK_FALSE,          // arrayOfPointers
                                                                     instanceAddress ); // data

    vk::AccelerationStructureGeometryKHR tlasGeometry( vk::GeometryTypeKHR::eInstances, // geometryType
                                                       instancesData,                   // geoemtry
                                                       { } );                           // flags

    vk::BuildAccelerationStructureModeKHR mode = reuse ? vk::BuildAccelerationStructureModeKHR::eUpdate : vk::BuildAccelerationStructureModeKHR::eBuild;

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo( vk::AccelerationStructureTypeKHR::eTopLevel, // type
                                                             flags,                                       // flags
                                                             mode,                                        // mode
                                                             nullptr,                                     // srcAccelerationStructure
                                                             { },                                         // dstAccelerationStructure
                                                             1,                                           // geometryCount
                                                             &tlasGeometry,                               // pGeometries
                                                             { },                                         // ppGeometries
                                                             { } );                                       // scratchData

    auto instancesCount = static_cast<uint32_t>( geometryInstances.size( ) );

    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo( { },   // accelerationStructureSize
                                                               { },   // updateScratchSize
                                                               { } ); // buildScratchSize

    vkCore::global::device.getAccelerationStructureBuildSizesKHR( vk::AccelerationStructureBuildTypeKHR::eDevice, &buildInfo, &instancesCount, &buildSizesInfo );

    if ( !reuse )
    {
      vk::AccelerationStructureCreateInfoKHR asCreateInfo( { },                                         // createFlags
                                                           { },                                         // buffer
                                                           { },                                         // offset
                                                           buildSizesInfo.accelerationStructureSize,    // size
                                                           vk::AccelerationStructureTypeKHR::eTopLevel, // type
                                                           { } );                                       // deviceAddress

      _tlas.as = initAccelerationStructure( asCreateInfo );
    }

    vk::MemoryAllocateFlagsInfo allocateInfo( vk::MemoryAllocateFlagBitsKHR::eDeviceAddress );

    vkCore::Buffer scratchBuffer( buildSizesInfo.buildScratchSize,                                                                           // size
                                  vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, // usage
                                  { vkCore::global::graphicsFamilyIndex },                                                                   // queueFamilyIndices
                                  vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent,                      // memoryPropertyFlags
                                  &allocateInfo );

    vk::BufferDeviceAddressInfo scratchBufferInfo( scratchBuffer.get( ) );
    vk::DeviceAddress scratchAddress = vkCore::global::device.getBufferAddress( &scratchBufferInfo );

    buildInfo.srcAccelerationStructure  = reuse ? _tlas.as.as : nullptr;
    buildInfo.dstAccelerationStructure  = _tlas.as.as;
    buildInfo.scratchData.deviceAddress = scratchAddress;

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo( instancesCount, // primitiveCount
                                                               0,              // primitiveOffset
                                                               0,              // firstVertex
                                                               0 );            // transformOffset

    const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &buildRangeInfo;

    cmdBuf.get( 0 ).buildAccelerationStructuresKHR( 1, &buildInfo, &pBuildRangeInfo );

    cmdBuf.end( 0 );
    cmdBuf.submitToQueue( vkCore::global::graphicsQueue );
  }

  void PathTracer::createStorageImage( vk::Extent2D swapchainExtent )
  {
    auto storageImageInfo   = vkCore::getImageCreateInfo( vk::Extent3D( swapchainExtent.width, swapchainExtent.height, 1 ) );
    storageImageInfo.usage  = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment;
    storageImageInfo.format = vk::Format::eB8G8R8A8Unorm; // TODO: make this the surface format, and not hard-coded

    _storageImage.init( storageImageInfo );
    _storageImage.transitionToLayout( vk::ImageLayout::eGeneral );

    _storageImageView = vkCore::initImageViewUnique( vkCore::getImageViewCreateInfo( _storageImage.get( ), _storageImage.getFormat( ) ) );

    auto samplerCreateInfo = vkCore::getSamplerCreateInfo( );
    _storageImageSampler   = vkCore::initSamplerUnique( samplerCreateInfo );

    _storageImageInfo.sampler     = _storageImageSampler.get( );
    _storageImageInfo.imageView   = _storageImageView.get( );
    _storageImageInfo.imageLayout = _storageImage.getLayout( );
  }

  void PathTracer::createShaderBindingTable( )
  {
    uint32_t groupHandleSize = _capabilities.pipelineProperties.shaderGroupHandleSize;
    uint32_t baseAlignment   = _capabilities.pipelineProperties.shaderGroupBaseAlignment;

    uint32_t sbtSize = _shaderGroups * baseAlignment;

    vk::MemoryAllocateFlagsInfo allocateFlags( vk::MemoryAllocateFlagBitsKHR::eDeviceAddress );

    _sbtBuffer.init( sbtSize,
                     vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                     { vkCore::global::graphicsFamilyIndex },
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     &allocateFlags );

    std::vector<uint8_t> shaderHandleStorage( sbtSize );
    auto result = vkCore::global::device.getRayTracingShaderGroupHandlesKHR( _pipeline.get( ),
                                                                             0,
                                                                             _shaderGroups,
                                                                             sbtSize,
                                                                             shaderHandleStorage.data( ) );

    RX_ASSERT( result == vk::Result::eSuccess, "Failed to get ray tracing shader group handles." );

    void* mapped = NULL;
    result       = vkCore::global::device.mapMemory( _sbtBuffer.getMemory( ), 0, _sbtBuffer.getSize( ), { }, &mapped );

    RX_ASSERT( result == vk::Result::eSuccess, "Failed to map memory for shader binding table." );

    auto* pData = reinterpret_cast<uint8_t*>( mapped );
    for ( uint32_t i = 0; i < _shaderGroups; ++i )
    {
      memcpy( pData, shaderHandleStorage.data( ) + i * groupHandleSize, groupHandleSize ); // raygen
      pData += baseAlignment;
    }

    vkCore::global::device.unmapMemory( _sbtBuffer.getMemory( ) );
  }

  void PathTracer::createPipeline( const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts )
  {
    //uint32_t anticipatedDirectionalLights = settings->maxDirectionalLights.has_value( ) ? settings->maxDirectionalLights.value( ) : components::maxDirectionalLights;
    //uint32_t anticipatedPointLights       = settings->maxPointLights.has_value( ) ? settings->maxPointLights.value( ) : components::maxPointLights;
    //Util::processShaderMacros( "shaders/PathTrace.rchit", anticipatedDirectionalLights, anticipatedPointLights, components::modelCount );

    auto rgen = vkCore::initShaderModuleUnique( components::assetsPath + "shaders/PathTrace.rgen", RX_GLSLC_PATH );
    auto miss = vkCore::initShaderModuleUnique( components::assetsPath + "shaders/PathTrace.rmiss", RX_GLSLC_PATH );
    auto chit = vkCore::initShaderModuleUnique( components::assetsPath + "shaders/PathTrace.rchit", RX_GLSLC_PATH );
    auto ahit = vkCore::initShaderModuleUnique( components::assetsPath + "shaders/PathTrace.rahit", RX_GLSLC_PATH );
    //auto ahit1 = vk::Initializer::initShaderModuleUnique( "shaders/PathTrace1.rahit" );
    auto missShadow = vkCore::initShaderModuleUnique( components::assetsPath + "shaders/PathTraceShadow.rmiss", RX_GLSLC_PATH );

    vk::PushConstantRange ptPushConstant( vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eClosestHitKHR, // stageFlags
                                          0,                                                                                                                 // offset
                                          sizeof( PtPushConstants ) );                                                                                       // size

    std::array<vk::PushConstantRange, 1> pushConstantRanges = { ptPushConstant };

    vk::PipelineLayoutCreateInfo layoutInfo( { },                                                   // flags
                                             static_cast<uint32_t>( descriptorSetLayouts.size( ) ), // setLayoutCount
                                             descriptorSetLayouts.data( ),                          // pSetLayouts
                                             static_cast<uint32_t>( pushConstantRanges.size( ) ),   // pushConstantRangeCount
                                             pushConstantRanges.data( ) );                          // pPushConstantRanges

    _layout = vkCore::global::device.createPipelineLayoutUnique( layoutInfo );
    RX_ASSERT( _layout.get( ), "Failed to create pipeline layout for path tracing pipeline." );

    std::array<vk::PipelineShaderStageCreateInfo, 5> shaderStages;
    shaderStages[0] = vkCore::getPipelineShaderStageCreateInfo( vk::ShaderStageFlagBits::eRaygenKHR, rgen.get( ) );
    shaderStages[1] = vkCore::getPipelineShaderStageCreateInfo( vk::ShaderStageFlagBits::eMissKHR, miss.get( ) );
    shaderStages[2] = vkCore::getPipelineShaderStageCreateInfo( vk::ShaderStageFlagBits::eMissKHR, missShadow.get( ) );
    shaderStages[3] = vkCore::getPipelineShaderStageCreateInfo( vk::ShaderStageFlagBits::eClosestHitKHR, chit.get( ) );
    shaderStages[4] = vkCore::getPipelineShaderStageCreateInfo( vk::ShaderStageFlagBits::eAnyHitKHR, ahit.get( ) );
    //shaderStages[4] = vkCore::getPipelineShaderStageCreateInfo( vk::ShaderStageFlagBits::eAnyHitKHR, ahit1.get( ) );

    // Set up path tracing shader groups.
    std::array<vk::RayTracingShaderGroupCreateInfoKHR, 4> groups;

    for ( auto& group : groups )
    {
      group.generalShader      = VK_SHADER_UNUSED_KHR;
      group.closestHitShader   = VK_SHADER_UNUSED_KHR;
      group.anyHitShader       = VK_SHADER_UNUSED_KHR;
      group.intersectionShader = VK_SHADER_UNUSED_KHR;
    }

    groups[0].generalShader = 0;
    groups[0].type          = vk::RayTracingShaderGroupTypeKHR::eGeneral;

    groups[1].generalShader = 1;
    groups[1].type          = vk::RayTracingShaderGroupTypeKHR::eGeneral;

    groups[2].generalShader = 2;
    groups[2].type          = vk::RayTracingShaderGroupTypeKHR::eGeneral;

    groups[3].closestHitShader = 3;
    groups[3].anyHitShader     = 4;
    groups[3].type             = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;

    //groups[3].closestHitShader = 4;
    //groups[3].anyHitShader = 3;
    //groups[3].type         = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;

    _shaderGroups = static_cast<uint32_t>( groups.size( ) );

    // @todo change hard-coded recursion depth
    vk::RayTracingPipelineCreateInfoKHR createInfo( { },                                           // flags
                                                    static_cast<uint32_t>( shaderStages.size( ) ), // stageCount
                                                    shaderStages.data( ),                          // pStages
                                                    static_cast<uint32_t>( groups.size( ) ),       // groupCount
                                                    groups.data( ),                                // pGroups
                                                    30,                                            // maxPipelineRayRecursionDepth
                                                    { },                                           // pLibraryInfo
                                                    nullptr,                                       // pLibraryInterface
                                                    { },                                           // pDynamicState
                                                    _layout.get( ),                                // layout
                                                    { },                                           // basePipelineHandle
                                                    0 );                                           // basePipelineIndex

    _pipeline = static_cast<vk::UniquePipeline>( vkCore::global::device.createRayTracingPipelineKHRUnique( { }, nullptr, createInfo ) );
    RX_ASSERT( _pipeline.get( ), "Failed to create path tracing pipeline." );
  }

  void PathTracer::pathTrace( vk::CommandBuffer swapchainCommandBuffer, vk::Image swapchainImage, vk::Extent2D extent )
  {
    vk::DeviceSize progSize = _capabilities.pipelineProperties.shaderGroupBaseAlignment;
    vk::DeviceSize sbtSize  = progSize * static_cast<vk::DeviceSize>( _shaderGroups );

    vk::DeviceAddress sbtAddress = vkCore::global::device.getBufferAddress( _sbtBuffer.get( ) );

    vk::StridedDeviceAddressRegionKHR bufferRegionRayGen( sbtAddress,     // deviceAddress
                                                          progSize,       // stride
                                                          progSize * 1 ); // size

    vk::StridedDeviceAddressRegionKHR bufferRegionMiss( sbtAddress + ( 1U * progSize ), // deviceAddress
                                                        progSize,                       // stride
                                                        progSize * 2 );                 // size

    vk::StridedDeviceAddressRegionKHR bufferRegionChit( sbtAddress + ( 3U * progSize ), // deviceAddress
                                                        progSize,                       // stride
                                                        progSize * 1 );                 // size

    vk::StridedDeviceAddressRegionKHR callableShaderBindingTable( 0,   // deviceAddress
                                                                  0,   // stride
                                                                  0 ); // size

    swapchainCommandBuffer.traceRaysKHR( &bufferRegionRayGen,         // pRaygenShaderBindingTable
                                         &bufferRegionMiss,           // pMissShaderBindingTable
                                         &bufferRegionChit,           // pHitShaderBindingTable
                                         &callableShaderBindingTable, // pCallableShaderBindingTable
                                         extent.width,                // width
                                         extent.height,               // height
                                         1 );                         // depth
  }

  void PathTracer::initDescriptorSet( )
  {
    // TLAS
    _descriptors.bindings.add( 0,
                               vk::DescriptorType::eAccelerationStructureKHR,
                               vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR );
    // Output image
    _descriptors.bindings.add( 1,
                               vk::DescriptorType::eStorageImage,
                               vk::ShaderStageFlagBits::eRaygenKHR );

#ifdef RX_VARIANCE_ESTIMATOR
    // Variance buffer
    _descriptors.bindings.add( 2,
                               vk::DescriptorType::eStorageBuffer,
                               vk::ShaderStageFlagBits::eRaygenKHR );
#endif

    _descriptors.layout = _descriptors.bindings.initLayoutUnique( );
    _descriptors.pool   = _descriptors.bindings.initPoolUnique( vkCore::global::swapchainImageCount );
    _descriptorSets     = vkCore::allocateDescriptorSets( _descriptors.pool.get( ), _descriptors.layout.get( ) );
  }

  void PathTracer::updateDescriptors( )
  {
    vk::WriteDescriptorSetAccelerationStructureKHR tlasInfo( 1,
                                                             &_tlas.as.as );

    vk::DescriptorBufferInfo varianceBufferInfo( _varianceBuffer.get( ),
                                                 0,
                                                 VK_WHOLE_SIZE );

    _descriptors.bindings.write( _descriptorSets, 0, &tlasInfo );
    _descriptors.bindings.write( _descriptorSets, 1, &_storageImageInfo );

#ifdef RX_VARIANCE_ESTIMATOR
    _descriptors.bindings.write( _descriptorSets, 2, &varianceBufferInfo );
#endif

    _descriptors.bindings.update( );
  }

  void PathTracer::initVarianceBuffer( float width, float height )
  {
    // Create buffer to store variance value
    _varianceBuffer.init( sizeof( float ) * 3 * width * height,
                          vk::BufferUsageFlagBits::eStorageBuffer,
                          { vkCore::global::graphicsFamilyIndex },
                          vk::MemoryPropertyFlagBits::eHostVisible );
  }

  // This is wrong! This calculates variance off all pixel colors of a frame.
  // Instead, one should look at the individual pixel at increased sample rates, right?
  float PathTracer::getPixelVariance( uint32_t index )
  {
    static bool alreadyMapped = false;
    static void* mapped       = NULL;

    if ( !alreadyMapped )
    {
      alreadyMapped     = true;
      vk::Result result = vkCore::global::device.mapMemory( _varianceBuffer.getMemory( ), 0, _varianceBuffer.getSize( ), { }, &mapped );
      RX_ASSERT( result == vk::Result::eSuccess, "Failed to map memory of variance buffer." );
    }

    auto* pData = reinterpret_cast<float*>( mapped );

    float res = ( pData )[index];
    return res;
  }
} // namespace RAYEX_NAMESPACE
