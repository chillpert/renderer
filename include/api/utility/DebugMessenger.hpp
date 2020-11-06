#pragma once

#include "pch/stdafx.hpp"

namespace RAYEX_NAMESPACE
{
  /// A wrapper class for a Vulkan debug utility messenger.
  ///
  /// The class features scope-bound destruction.
  /// @ingroup API
  class DebugMessenger
  {
  public:
    DebugMessenger( ) = default;

    /// Calls destroy().
    ~DebugMessenger( );

    DebugMessenger( const DebugMessenger& )  = delete;
    DebugMessenger( const DebugMessenger&& ) = delete;

    auto operator=( const DebugMessenger& ) -> DebugMessenger& = delete;
    auto operator=( const DebugMessenger&& ) -> DebugMessenger& = delete;

    /// Creates the debug messenger with the given properties.
    /// @param messageSeverity - Specifies the type of severity of messages that will be logged.
    /// @param messageType - Specifies the types of messages that will be logged.
    void init( vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError, vk::DebugUtilsMessageTypeFlagsEXT messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );

  private:
    /// Destroys the debug messenger.
    void destroy( );

    vk::DebugUtilsMessengerEXT _debugMessenger;
  };

  /// @cond INTERNAL
  VKAPI_ATTR auto VKAPI_CALL debugMessengerCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                     void* userData ) -> VkBool32;
  /// @endcond
} // namespace RAYEX_NAMESPACE
