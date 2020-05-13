#include "Instance.hpp"

namespace RX
{
  void Instance::pushLayer(const char* name)
  {
    checkLayerSupport(name);

    layers.push_back(name);
    VK_LOG("Added Layer: " << name);
  }

  void Instance::pushExtension(const char* name)
  {
    checkExtensionSupport(name);

    extensions.push_back(name);
    VK_LOG("Added Extension: " << name);
  }

  void Instance::create(const std::shared_ptr<Window> window)
  {
    VkApplicationInfo appInfo{ };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = getApiVersion();

    VkInstanceCreateInfo createInfo{ };
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

#ifdef RX_DEBUG
#if RX_PLATFORM_WINDOWS_X64
    pushLayer("VK_LAYER_KHRONOS_validation");
#elif RX_PLATFORM_UNIX_X64
    pushLayer("VK_LAYER_LUNARG_standard_validation");
#endif
    createInfo.ppEnabledLayerNames = layers.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
#endif

    // Retrieves all extensions needed by SDL2
    uint32_t sdlExtensionsCount;
    window->getInstanceExtensions(sdlExtensionsCount, NULL);

    const char** sdlExtensionsNames = new const char* [sdlExtensionsCount];

    window->getInstanceExtensions(sdlExtensionsCount, sdlExtensionsNames);

    for (size_t i = 0; i < sdlExtensionsCount; ++i)
    {
      pushExtension(sdlExtensionsNames[i]);
    }

#ifdef RX_DEBUG
    pushExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

    VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &instance), "Failed to create instance");
  }

  void Instance::destroy()
  {
    vkDestroyInstance(instance, nullptr);
  }

  void Instance::print()
  {
    uint32_t layerCount;
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&layerCount, nullptr), "Failed to enumerate instance layer properties");

    std::vector<VkLayerProperties> layers(layerCount);
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&layerCount, layers.data()), "Failed to enumerate instance layer properties");
    
    VK_LOG("\n\nAvailable extensions on this device:");
    std::cout << "==================================================================\n";
    for (const auto& layer : layers)
      std::cout << "  " << layer.layerName << ":\n\t" << "Description: " << layer.description << std::endl;
    std::cout << std::endl;

    uint32_t extensionCount;
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr), "Failed to enumerate instance extension properties");

    std::vector<VkExtensionProperties> extensions(extensionCount);
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()), "Failed to enumerate instance extension properties");

    VK_LOG("\n\nAvailable layers on this device:");
    std::cout << "==================================================================\n";
    for (const auto& extension : extensions)
      std::cout << "  " << extension.extensionName << std::endl;
    std::cout << std::endl;
  }

  void Instance::checkLayerSupport(const char* name)
  {
    uint32_t propertyCount;
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&propertyCount, nullptr), "Failed to enumerate instance layer properties");

    std::vector<VkLayerProperties> properties(propertyCount);
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&propertyCount, properties.data()), "Failed to enumerate instance layer properties");

    for (const auto& property : properties)
    {
      if (strcmp(property.layerName, name) == 0)
        return;
    }
    
    VK_ERROR("Validation layer is not available on this device");
  }

  void Instance::checkExtensionSupport(const char* name)
  {
    uint32_t propertyCount;
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr), "Failed to enumerate instance extension properties");

    std::vector<VkExtensionProperties> properties(propertyCount);
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, properties.data()), "Failed to enumerate instance extension properties");

    for (const auto& property : properties)
    {
      if (strcmp(property.extensionName, name) == 0)
        return;
    }

    VK_ERROR("Validation layer is not available on this device");
  }

  uint32_t Instance::getApiVersion()
  {
    uint32_t apiVersion;
    VK_ASSERT(vkEnumerateInstanceVersion(&apiVersion), "Failed to enumerate instance version");

    //std::cout << "cool : " << VK_VERSION_MINOR(apiVersion) << std::endl;

#ifdef VK_API_VERSION_1_2
    if (apiVersion >= VK_API_VERSION_1_2)
    {
      VK_LOG("Found Vulkan SDK API Version 1.2.x");
      return apiVersion;
    }
#endif

#ifdef VK_API_VERSION_1_1  _
    if (apiVersion >= VK_API_VERSION_1_1)
    {
      VK_LOG("Found Vulkan SDK API Version 1.1.x");
      return apiVersion;
    }
#endif
    
    VK_ERROR("This application requires Vulkan SDK API Version 1.1 or higher");
  
    return 0;
  }
}