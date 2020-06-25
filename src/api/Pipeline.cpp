#include "Pipeline.hpp"
#include "Vertex.hpp"
#include "Initializers.hpp"

namespace RX
{
  Pipeline::~Pipeline()
  {
    if (m_pipeline)
      destroy();
  }

  void Pipeline::initialize(RasterizationPipelineInfo& info)
  {
    m_info = info;

    // TODO: this has to be more adjustable.
    auto bindingDescription = Vertex::getBindingDescriptions();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = info.topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &m_info.viewport; // TODO: spec states if the viewport is dynamic this is ignored.
    viewportState.scissorCount = 1;
    viewportState.pScissors = &m_info.scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::array<vk::DynamicState, 2> dynamicStates{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    vk::PushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::vec3);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &info.descriptorSetLayout;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    m_layout = m_info.device.createPipelineLayout(pipelineLayoutInfo);
    if (!m_layout)
      RX_ERROR("Failed to create pipeline layout.");

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = info.vertexShader;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = info.fragmentShader;
    fragShaderStageInfo.pName = "main";

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

    vk::GraphicsPipelineCreateInfo createInfo;
    createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = &vertexInputInfo;
    createInfo.pInputAssemblyState = &inputAssembly;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterizer;
    createInfo.pMultisampleState = &multisampling;
    createInfo.pDepthStencilState = &depthStencil;
    createInfo.pColorBlendState = &colorBlending;
    createInfo.layout = m_layout;
    createInfo.renderPass = m_info.renderPass;
    createInfo.subpass = 0;
    createInfo.basePipelineHandle = nullptr;
    createInfo.pDynamicState = &dynamicStateInfo;

    m_pipeline = m_info.device.createGraphicsPipeline(nullptr, createInfo, nullptr);
    if (!m_pipeline)
      RX_ERROR("Failed to create graphics pipeline."); 
  }

  void Pipeline::initialize(RaytracingPipelineInfo& info)
  {
    m_info = info;

    vk::PipelineLayoutCreateInfo layoutInfo
    {
      { },                                        // flags
      static_cast<uint32_t>(info.layouts.size()), // setLayoutCount
      info.layouts.data(),                        // pSetLayouts
      0,                                          // pushConstantRangeCount
      nullptr                                     // pPushConstantRanges
    };

    m_layout = m_info.device.createPipelineLayout(layoutInfo);
    if (!m_layout)
      RX_ERROR("Failed to create pipeline layout.");

    vk::SpecializationMapEntry specializationMapEntry
    {
      0,                // constantID
      0,                // offset
      sizeof(uint32_t)  // size
    };

    vk::SpecializationInfo specializationInfo
    {
       1,                         // mapEntryCount
       &specializationMapEntry,   // pMapEntries
       sizeof(info.maxRecursion), // dataSize
       &info.maxRecursion         // pData
    };

    std::array<vk::PipelineShaderStageCreateInfo, 3> shaderStages;
    shaderStages[0] = Initializers::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eRaygenKHR, info.rayGen->get());
    shaderStages[1] = Initializers::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eMissKHR, info.miss->get());
    shaderStages[2] = Initializers::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eClosestHitKHR, info.closestHit->get());

    // Set up raytracing shader groups.
    std::array<vk::RayTracingShaderGroupCreateInfoKHR, 3> groups;

    groups[0].generalShader = 0;
    
    groups[1].generalShader = 1;
    
    groups[2].closestHitShader = 2;

    vk::RayTracingPipelineCreateInfoKHR createInfo{
      { },                                        // flags
      static_cast<uint32_t>(shaderStages.size()), // stageCount
      shaderStages.data(),                        // pStages
      static_cast<uint32_t>(groups.size()),       // groupCount
      groups.data(),                              // pGroups
      info.maxRecursion,                          // maxRecursionDepth
      0,                                          // libraries
      nullptr,                                    // pLibraryInterface
      m_layout,                                   // layout
      nullptr,                                    // basePipelineHandle
      0                                           // basePipelineIndex
    };

    auto result = m_info.device.createRayTracingPipelineKHR(nullptr, createInfo, nullptr, info.dispatchLoaderDynamic);
    if (result.result != vk::Result::eSuccess)
      RX_ERROR("Failed to create ray tracing pipeline.");

    m_pipeline = result.value;
  }

  void Pipeline::destroy()
  {
    m_info.device.destroyPipeline(m_pipeline);
    m_pipeline = nullptr;

    m_info.device.destroyPipelineLayout(m_layout);
    m_layout = nullptr;
  }
}
