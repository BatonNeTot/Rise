//☀Rise☀
#include "Rise/rise.h"

#include "Rise/logger.h"
#include "Rise/loader.h"
#include "Rise/resource_manager.h"
#include "Rise/gpu_allocator.h"
#include "Rise/window.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include FT_MODULE_H

#include <iostream>
#include <set>
#include <unordered_map>

static const std::vector<const char*> vulkanValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> vulkanDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_MAINTENANCE_3_EXTENSION_NAME
};

static const std::set<std::string> vulkanOptionalDeviceExtensions = {
    "VK_KHR_portability_subset",
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
};

#ifdef NDEBUG
    static const bool enableVulkanValidationLayers = false;
#else
    static const bool enableVulkanValidationLayers = true;
#endif

namespace Rise {
    
static Core* _pInstance = nullptr;

Core* Instance() {
    return Core::Instance();
}

bool Init() {
    return Core::Init();
}

void Destroy() {
    Core::Destroy();
}

Core* Core::Instance() {
    return _pInstance;
}

bool Core::Init() {
    if (Instance()) {
        return false;
    }
    
    _pInstance = new Core();
    
    return true;
}

void Core::Destroy() {
    if (!Instance()) {
        return;
    }
    
    delete _pInstance;
    _pInstance = nullptr;
}

Core::Core() {
    _logger = new Rise::Logger(Rise::Logger::Level::Trace);

    InitGLFW();
    InitVulkan();

    _loader = new Rise::Loader(8);
    _resources = new Rise::ResourceManager();
    _resourceGenerator = new Rise::ResourceGenerator(*_resources, *_loader);

    _resourceGenerator->RegisterBuiltinResources();

    if (FT_Init_FreeType(&_freeTypeLibrary))
    {
        Logger().Error("couldn't initialize freeType!");
    }

    _globalFrameCounter = 0;
}

void Core::InitGLFW() {
    glfwInit();
}

void Core::InitVulkan() {
    CreateVInstance();
    SetupDebugMessenger();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateCommandPool();
}

void Core::CreateVInstance() {
    if (enableVulkanValidationLayers && !CheckVulkanValidationLayerSupport()) {
        Logger().Error("validation layers requested, but not available!");
    }
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Rise Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableVulkanValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(vulkanValidationLayers.size());
        createInfo.ppEnabledLayerNames = vulkanValidationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }
    
    if (vkCreateInstance(&createInfo, nullptr, &_vInstance) != VK_SUCCESS) {
        Logger().Error("failed to create instance!");
    }
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    auto& core = *reinterpret_cast<Core*>(pUserData);
    Logger::Level level = Logger::Level::Debug;
    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        level = Logger::Level::Error;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        level = Logger::Level::Warning;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        level = Logger::Level::Info;
        break;
    }
    core.Logger().Log(level, std::string("validation layer: ") + pCallbackData->pMessage);
    return VK_FALSE;
}

void Core::SetupDebugMessenger() {
    if (!enableVulkanValidationLayers) return;
    
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugMessengerCreateInfo(createInfo);
    
    if (CreateDebugUtilsMessengerEXT(_vInstance, &createInfo, nullptr, &_vDebugMessenger) != VK_SUCCESS) {
        Logger().Error("failed to set up debug messenger!");
    }
}

void Core::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = this;
}

bool Core::CheckVulkanValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    for (const auto& layerName : vulkanValidationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char *> Core::GetRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableVulkanValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

GLFWwindow* Core::CreateTemporaryWindow() {
    glfwDefaultWindowHints();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);
    
    return glfwCreateWindow(16, 16, "Temporary", nullptr, nullptr);
}

void Core::CreateTemporarySurface(VkSurfaceKHR& surface, GLFWwindow* pWindow) {
    if (glfwCreateWindowSurface(_vInstance, pWindow, nullptr, &surface) != VK_SUCCESS) {
        Logger().Error("failed to create window surface!");
    }
}

void Core::PickPhysicalDevice() {
    GLFWwindow* pWindow = CreateTemporaryWindow();
    VkSurfaceKHR surface;
    CreateTemporarySurface(surface, pWindow);
    
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_vInstance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        Logger().Error("failed to find GPUs with Vulkan support!");
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_vInstance, &deviceCount, devices.data());
    
    for (const auto& device : devices) {
        if (IsDeviceSuitable(device, surface)) {
            _vPhysicalDevice = device;
            break;
        }
    }

    if (_vPhysicalDevice == VK_NULL_HANDLE) {
        Logger().Error("failed to find a suitable GPU!");
    }
    
    _queueFamilyIndices = FindQueueFamilies(_vPhysicalDevice, surface);
    
    vkDestroySurfaceKHR(_vInstance, surface, nullptr);
    glfwDestroyWindow(pWindow);
}

bool Core::IsDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
    QueueFamilyIndices indices = FindQueueFamilies(device, surface);
    
    bool extensionsSupported = CheckDeviceExtensionSupport(device);
    
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.IsComplete() && extensionsSupported && swapChainAdequate;
}

bool Core::CheckDeviceExtensionSupport(const VkPhysicalDevice& device) {
    static std::unordered_map<VkPhysicalDevice, bool> support;
    
    auto it = support.find(device);
    if (it == support.end()) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(vulkanDeviceExtensions.begin(), vulkanDeviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        
        auto emplace_result = support.emplace(device, requiredExtensions.empty());
        it = emplace_result.first;
    }
    return it->second;
}

void Core::AddOptionalExtensions(std::vector<const char*>& deviceExtensions) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(_vPhysicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(_vPhysicalDevice, nullptr, &extensionCount, availableExtensions.data());
    
    for (const auto& extension : availableExtensions) {
        auto it = vulkanOptionalDeviceExtensions.find(extension.extensionName);
        if (it != vulkanOptionalDeviceExtensions.end()) {
            deviceExtensions.emplace_back(it->c_str());
        }
    }
}

Core::QueueFamilyIndices Core::FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
    QueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }
        
        if (indices.IsComplete()) {
            break;
        }

        i++;
    }
    
    return indices;
}

Core::SwapChainSupportDetails Core::QuerySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
    SwapChainSupportDetails details;
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void Core::CreateLogicalDevice() {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {_queueFamilyIndices.graphicsFamily.value(), _queueFamilyIndices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features.fillModeNonSolid = true;
    deviceFeatures2.features.samplerAnisotropy = true;

    VkPhysicalDeviceVulkan12Features deviceFeatures12{};
    deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    deviceFeatures12.descriptorIndexing = true;
    deviceFeatures12.runtimeDescriptorArray = true;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    deviceFeatures2.pNext = &deviceFeatures12;
    createInfo.pEnabledFeatures = nullptr;
    createInfo.pNext = &deviceFeatures2;
    
    std::vector<const char*> deviceExtensions(vulkanDeviceExtensions.begin(), vulkanDeviceExtensions.end());
    //AddOptionalExtensions(deviceExtensions);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableVulkanValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(vulkanValidationLayers.size());
        createInfo.ppEnabledLayerNames = vulkanValidationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(_vPhysicalDevice, &createInfo, nullptr, &_vDevice) != VK_SUCCESS) {
        Logger().Error("failed to create logical device!");
    }

    vkGetDeviceQueue(_vDevice, _queueFamilyIndices.graphicsFamily.value(), 0, &_vGraphicsQueue);
    vkGetDeviceQueue(_vDevice, _queueFamilyIndices.presentFamily.value(), 0, &_vPresentQueue);
}

void Core::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = _queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(_vDevice, &poolInfo, nullptr, &_vCommandPool) != VK_SUCCESS) {
        Logger().Error("failed to create command pool!");
    }
    if (vkCreateCommandPool(_vDevice, &poolInfo, nullptr, &_vInstantCommandPool) != VK_SUCCESS) {
        Logger().Error("failed to create instant command pool!");
    }
}

Core::~Core() {
    for (auto& pWindow : _sWindows) {
        delete pWindow;
    }
    _sWindows.clear();

    FT_Done_Library(_freeTypeLibrary);

    delete _resourceGenerator;
    _resourceGenerator = nullptr;

    delete _resources;
    _resources = nullptr;

    delete _loader;
    _loader = nullptr;

    std::lock_guard<std::recursive_mutex> lg(_deviceLock);
    GpuAllocator::Destroy();

    vkDestroyCommandPool(_vDevice, _vInstantCommandPool, nullptr);
    vkDestroyCommandPool(_vDevice, _vCommandPool, nullptr);
    vkDestroyDevice(_vDevice, nullptr);
    
    if (enableVulkanValidationLayers) {
        DestroyDebugUtilsMessengerEXT(_vInstance, _vDebugMessenger, nullptr);
    }
    
    vkDestroyInstance(_vInstance, nullptr);
    glfwTerminate();

    delete _logger;
    _logger = nullptr;
}

Window* Core::ConstructWindow(const std::string& id, int iWidth, int iHeight) {
    static Window::Meta windowMeta("default");
    auto* pWindow = new Window(this, &windowMeta);
    pWindow->SetSize({static_cast<uint32_t>(iWidth), static_cast<uint32_t>(iHeight)});
    _sWindows.emplace(pWindow);
    return pWindow;
}

bool Core::DestroyWindow(Window* pWindow) {
    if (!_sWindows.erase(pWindow)) {
        return false;
    }

    {
        std::lock_guard<std::recursive_mutex> lg(_deviceLock);
        vkDeviceWaitIdle(_vDevice);
    }
    
    delete pWindow;
    
    return true;
}

void Core::Loop() {
    while(!_sWindows.empty()) {
        std::vector<Window*> vShouldClose;
        
        for (auto& pWindow : _sWindows) {
            if (pWindow->ShouldClose()) {
                vShouldClose.emplace_back(pWindow);
            }
        }
        
        for (auto& pWindow : vShouldClose) {
            DestroyWindow(pWindow);
        }
        
        if (_sWindows.empty()) {
            break;
        }
        
        for (auto& pWindow : _sWindows) {
            pWindow->LoopStep();
        }
        
        glfwPollEvents();

        ++_globalFrameCounter;
    }
}
    
}
