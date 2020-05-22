#include "api/DebugMessenger.hpp"

namespace RX
{
  DebugMessenger::DebugMessenger() :
    BaseComponent("DebugMessenger") { }

  DebugMessenger::~DebugMessenger()
  {
    destroy();
  }

  void DebugMessenger::initialize(VkInstance instance)
  {
#ifdef RX_DEBUG
    m_instance = instance;

    m_createDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    m_destroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    VkDebugUtilsMessengerCreateInfoEXT createInfo{ };
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
      //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    createInfo.pfnUserCallback = debugMessengerCallback;
    
    VK_ASSERT(m_createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &m_debugMessenger), "Failed to create debug utils messenger");

    initializationCallback();
#endif
  }

  void DebugMessenger::destroy()
  {
#ifdef RX_DEBUG
    assertDestruction();
    m_destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif
  }

  // The source for this function is the official LunarG tutorial.
  VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback
  (
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData
  )
  {
    char prefix[64];
    char* message = static_cast<char*>(malloc(strlen(callbackData->pMessage) + 500));
    assert(message);

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
      strcpy_s(prefix, "VERBOSE: ");
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
      strcpy_s(prefix, "INFO: ");
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
      strcpy_s(prefix, "WARNING: ");
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
      strcpy_s(prefix, "ERROR: ");
    }

    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
      strcat_s(prefix, "GENERAL");
    }
    else
    {
      if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
      {
        strcat_s(prefix, "VALIDATION");
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        {
          strcat_s(prefix, " | PERFORMANCE");
        }
      }
    }

    sprintf
    (
      message,
      "%s \n\tID Number: %d\n\tID String: %s \n\tMessage:   %s",
      prefix,
      callbackData->messageIdNumber,
      callbackData->pMessageIdName,
      callbackData->pMessage
    );

    printf("%s\n", message);
    fflush(stdout);
    free(message);

    return VK_FALSE;
  }
}