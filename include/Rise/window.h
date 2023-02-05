//☀Rise☀
#ifndef window_h
#define window_h

#include "image.h"
#include "render_target.h"

#include <windows.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace Rise {

    class Window : public IRenderTarget {
    public:

        constexpr static const char* Ext = "wndw";

        friend Core;
        friend class GraphicsPipeline;

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT 2
#endif

        bool Opened() const;
        bool Open();
        void Close();

        uint32_t FrameNumber() const {
            return _frameNumber;
        }

        const IImage::Meta* imageMeta() const override {
            return &_swapchainMeta;
        }

        class Meta {
        public:

            Meta(const std::string& id)
                : _id(id) {}

            std::string _id;
        
        };

    private:

        IImage* CurrentImage() override {
            return &_swapchainImages[_currentImageIndex];
        }

        Window(Core* core, Meta* meta);
        ~Window();

        void SetSize(const Size2D& size) { _size = size; }

        bool ShouldClose();
        void LoopStep();

        static void FramebufferResizeCallback(GLFWwindow* pWindow, int iWidth, int iHeight);
        static void MouseButtonCallback(GLFWwindow* pWindow, int button, int action, int mods);

        void CreateSwapChain();
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        void CleanupSwapChain();
        void RecreateSwapChain();

        void SyncObjects();

        void DrawFrame();

        const Point2D& GetPos() override {
            return Point2D::zero;
        }
        const Size2D& GetSize() override {
            checkDirtyMetrics();
            return _size;
        }

        Size2D _size;

        Meta* _meta = nullptr;

    private:

        size_t GetProperImageDataIndex() const override;

        std::vector<VkSemaphore> WaitSemaphores() const override {
            return _vImageAvailableSemaphore;
        }

        GLFWwindow* _pWindow = nullptr;

        VkSurfaceKHR _vSurface;
        VkSwapchainKHR _vSwapChain;

        uint32_t _currentImageIndex = 0;
        std::vector<SwapchainImage> _swapchainImages;

        IImage::Meta _swapchainMeta;

        std::vector<VkSemaphore> _vImageAvailableSemaphore = std::vector<VkSemaphore>(MAX_FRAMES_IN_FLIGHT);

        std::atomic_uint32_t _frameNumber = 0;
        bool _framebufferResized = false;

    };

}

#endif /* window_h */
