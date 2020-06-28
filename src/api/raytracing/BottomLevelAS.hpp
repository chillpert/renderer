#ifndef BOTTOM_LEVEL_AS_HPP
#define BOTTOM_LEVEL_AS_HPP

#include "Base.hpp"
#include "Queue.hpp"

namespace RX
{
  struct BottomLevelASInfo
  {
    vk::BuildAccelerationStructureFlagsKHR flags;
    std::shared_ptr<Queue> queue;
  };

  class BottomLevelAS
  {
  public:
    ~BottomLevelAS();
    void destroy();

    inline vk::AccelerationStructureKHR& get() { return m_as; }

    // Creates a single bottom level AS.
    void init(BottomLevelASInfo& info, const std::shared_ptr<Model> model, std::vector<BottomLevelAS>& blas);

    // Creates bottom level AS for all given models.
    friend void initBottomLevelAS_(BottomLevelASInfo& info, const std::vector<std::shared_ptr<Model>>& models, std::vector<BottomLevelAS>& blas_);
    // Builds all bottom level AS for all given models.
    friend void buildBottomLevelAS_(BottomLevelASInfo& info, const std::vector<std::shared_ptr<Model>>& models, std::vector<BottomLevelAS>& blas_);

  private:
    BottomLevelASInfo m_info;

    vk::AccelerationStructureKHR m_as;
    vk::DeviceMemory m_memory;
    uint64_t m_handle;
  };
}

#endif // BOTTOM_LEVEL_AS_HPP