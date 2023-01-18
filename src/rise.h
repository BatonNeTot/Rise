//☀Rise☀
#ifndef rise_rise_h
#define rise_rise_h

#include <windows.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <unordered_set>
#include <vector>
#include <optional>
#include <mutex>

#ifndef RISE_RESOURCE_DIRECTORY
#define RISE_RESOURCE_DIRECTORY ""
#endif

namespace Rise {

    class Window;

    class Logger;
    class Loader;
    class ResourceManager;
    class ResourceGenerator;

    class Core {
    public:

        static Core* Instance();

        static bool Init();
        static void Destroy();

        Window* ConstructWindow(const std::string& id, int iWidth, int iHeight);
        bool DestroyWindow(Window* pWindow);

        void Loop();

        Rise::ResourceGenerator& ResourceGenerator() {
            return *_resourceGenerator;
        }
        Rise::ResourceManager& Resources() {
            return *_resources;
        }
        Rise::Loader& Loader() {
            return *_loader;
        }
        Rise::Logger& Logger() {
            return *_logger;
        }

    private:

        friend Window;
        friend class GpuAllocator;
        friend class GpuStackAllocator;
        friend class GraphicsPipeline;
        friend class Resource;

        friend class RenderPass;
        friend class Renderer;
        friend class Framebuffer;

        friend class Vertices;
        friend class CustomVertices;

        friend class IRenderTarget;
        friend class RenderTarget;

        friend class Sampler;
        friend class IImage;
        friend class Image;
        friend class CustomImage;

        friend class IShader;

        friend class Font;

        friend class Uniform;
        friend class BufferUniformSetter;
        friend class SamplerUniformSetter;
        friend class ImageUniformSetter;
        friend class ImagesUniformSetter;

        Core();
        ~Core();

        void InitGLFW();

        void InitVulkan();
        void CreateVInstance();

        void SetupDebugMessenger();
        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

        bool CheckVulkanValidationLayerSupport();
        std::vector<const char*> GetRequiredExtensions();

        GLFWwindow* CreateTemporaryWindow();
        void CreateTemporarySurface(VkSurfaceKHR& surface, GLFWwindow* pWindow);

        void PickPhysicalDevice();
        bool IsDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
        bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device);
        void AddOptionalExtensions(std::vector<const char*>& deviceExtensions);

        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            bool IsComplete() {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };
        QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

        void CreateLogicalDevice();

        void CreateCommandPool();

        std::unordered_set<Window*> _sWindows;

        VkInstance _vInstance;
        VkDebugUtilsMessengerEXT _vDebugMessenger;

        VkPhysicalDevice _vPhysicalDevice = VK_NULL_HANDLE;

        VkDevice _vDevice;
        std::recursive_mutex _deviceLock;

        QueueFamilyIndices _queueFamilyIndices;

        VkQueue _vGraphicsQueue;
        VkQueue _vPresentQueue;

        VkCommandPool _vCommandPool;
        VkCommandPool _vInstantCommandPool;

        Rise::Logger* _logger = nullptr;
        Rise::Loader* _loader = nullptr;
        Rise::ResourceManager* _resources = nullptr;
        Rise::ResourceGenerator* _resourceGenerator = nullptr;

        FT_Library  _freeTypeLibrary;

        uint64_t _globalFrameCounter = 0;
    };

    Core* Instance();

    bool Init();
    void Destroy();

}

#endif /* rise_rise_h */
