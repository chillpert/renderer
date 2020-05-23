#include "CommandBuffers.hpp"
#include "Buffers/Vertex.hpp"

namespace RX
{
	CommandBuffers::CommandBuffers() :
		BaseComponent("CommandBuffers") { }

	void CommandBuffers::initialize(VkDevice device, VkCommandPool commandPool, size_t swapchainFramebufferSize)
	{
		m_device = device;
		m_commandPool = commandPool;

	  m_commandBuffers.resize(swapchainFramebufferSize);

	  VkCommandBufferAllocateInfo allocateInfo{ };
	  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	  allocateInfo.commandPool = commandPool;
	  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	  allocateInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	  VK_ASSERT(vkAllocateCommandBuffers(device, &allocateInfo, m_commandBuffers.data()), "Failed to allocate command buffers");
	
		initializationCallback();
	}

	void CommandBuffers::record(Swapchain& swapchain, Framebuffers& framebuffers, RenderPass& renderPass, Pipeline& pipeline, Buffer& vertexBuffer, Buffer& indexBuffer)
	{
		assertInitialized("record");

		for (size_t i = 0; i < m_commandBuffers.size(); ++i)
		{
	    VkCommandBufferBeginInfo beginInfo{ };
	    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	    VK_ASSERT(vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo), "Failed to record command buffers");

	    VkRenderPassBeginInfo renderPassInfo{ };
	    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	    renderPassInfo.renderPass = renderPass.get();
	    renderPassInfo.framebuffer = framebuffers.get()[i];
	    renderPassInfo.renderArea.offset = { 0, 0 };
	    renderPassInfo.renderArea.extent = swapchain.getExtent();

	    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	    renderPassInfo.clearValueCount = 1;
	    renderPassInfo.pClearValues = &clearColor;

	    vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get());

				VkBuffer vertexBuffers[] = { vertexBuffer.get() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(m_commandBuffers[i], indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32); // TODO: get data type from index buffer

				//vkCmdDraw(m_commandBuffers[i], vertexBuffer.getVertexCount(), 1, 0, 0); // TODO: vertices should be passed to this function
				vkCmdDrawIndexed(m_commandBuffers[i], 6, 1, 0, 0, 0); // TODO: 6 is hard-coded size of indices in index buffer

	    vkCmdEndRenderPass(m_commandBuffers[i]);

	    VK_ASSERT(vkEndCommandBuffer(m_commandBuffers[i]), "Failed to record command buffers");
    }
	}

	void CommandBuffers::free()
	{
		assertDestruction();
		vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
	}
}