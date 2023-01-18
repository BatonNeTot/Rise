//☀Rise☀
#include "window.h"

#include "rise.h"

#include "gpu_allocator.h"
#include "resource.h"

#include <cstdint>
#include <limits>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <iostream>
#include <chrono>

namespace Rise {

    Window::Window(Core* core, Meta* meta)
        : IRenderTarget(core), _meta(meta) {}

    Window::~Window() {
        Close();
    }

    bool Window::Opened() const {
        return _pWindow;
    }

    bool Window::Open() {
        if (Opened()) {
            return false;
        }

        if (!Instance()) {
            return false;
        }

        glfwDefaultWindowHints();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        _pWindow = glfwCreateWindow(_size.width, _size.height, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(_pWindow, this);

        glfwSetFramebufferSizeCallback(_pWindow, &Window::FramebufferResizeCallback);
        glfwSetMouseButtonCallback(_pWindow, &Window::MouseButtonCallback);

        if (glfwCreateWindowSurface(Instance()->_vInstance, _pWindow, nullptr, &_vSurface) != VK_SUCCESS) {
            Error("failed to create window surface!");
        }

        CreateSwapChain();
        SyncObjects();

        auto absoluteFilename = Instance()->ResourceGenerator().GetFullPath<Window>(_meta->_id);
        std::ifstream i(absoluteFilename);

        Data data;
        i >> data;

        instantiateNode(data);

        return true;
    }

    void Window::FramebufferResizeCallback(GLFWwindow* pWindow, int iWidth, int iHeight) {
        auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pWindow));
        window->_size.width = static_cast<uint32_t>(iWidth);
        window->_size.height = static_cast<uint32_t>(iHeight);
        window->_framebufferResized = true;
        window->raiseDirtyMetrics();
    }

    void Window::MouseButtonCallback(GLFWwindow* pWindow, int button, int action, int mods) {
        auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pWindow)); 
        if (action == GLFW_RELEASE) {
            double xpos, ypos;
            glfwGetCursorPos(pWindow, &xpos, &ypos);
            window->onMouseClickImpl(button == GLFW_MOUSE_BUTTON_LEFT, {
                static_cast<int32_t>(xpos),
                static_cast<int32_t>(ypos) });
        }
    }

    void Window::CreateSwapChain() {
        Core::SwapChainSupportDetails swapChainSupport = Instance()->QuerySwapChainSupport(Instance()->_vPhysicalDevice, _vSurface);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = _vSurface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        Core::QueueFamilyIndices indices = Instance()->FindQueueFamilies(Instance()->_vPhysicalDevice, _vSurface);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        std::vector<VkImage> swapchainImages;
        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            if (vkCreateSwapchainKHR(Instance()->_vDevice, &createInfo, nullptr, &_vSwapChain) != VK_SUCCESS) {
                Error("failed to create swap chain!");
            }

            vkGetSwapchainImagesKHR(Instance()->_vDevice, _vSwapChain, &imageCount, nullptr);
            swapchainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(Instance()->_vDevice, _vSwapChain, &imageCount, swapchainImages.data());
        }

        _swapchainMeta.size = extent;
        _swapchainMeta.vFormat = surfaceFormat.format;

        _swapchainImages = std::vector<SwapchainImage>(swapchainImages.size());
        for (auto i = 0; i < _swapchainImages.size(); ++i) {
            _swapchainImages[i].SetupMeta(_swapchainMeta);
            _swapchainImages[i].Load(swapchainImages[i], VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    VkSurfaceFormatKHR Window::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR Window::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D Window::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        int width, height;
        glfwGetFramebufferSize(_pWindow, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }

    void Window::CleanupSwapChain() {
        std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
        _swapchainImages.clear();

        vkDestroySwapchainKHR(Instance()->_vDevice, _vSwapChain, nullptr);
    }

    void Window::RecreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(_pWindow, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(_pWindow, &width, &height);
            glfwWaitEvents();
        }

        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            vkDeviceWaitIdle(Instance()->_vDevice);
        }

        CleanupSwapChain();

        CreateSwapChain();

        /*
        for (auto& renderPass : _renderPasses) {
            renderPass.second.UpdateExtent(_swapchainMeta.size, _swapchainImageViews);
        }
        */
    }

    void Window::SyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(Instance()->_vDevice, &semaphoreInfo, nullptr, &_vImageAvailableSemaphore[i]) != VK_SUCCESS) {
                Error("failed to create semaphores!");
            }
        }
    }

    void Window::Close() {
        if (!Opened()) {
            return;
        }

        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            vkDeviceWaitIdle(Instance()->_vDevice);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(Instance()->_vDevice, _vImageAvailableSemaphore[i], nullptr);
            }
            CleanupSwapChain();
        }

        ClearChildren();

        /*
        for (auto& [renderPassId, renderPass] : _renderPasses) {
            renderPass.Unuse();
            while (!renderPass.Unloaded()) {
                Rise::Instance()->Resources().CollectGarbage();
            };
        }
        */

        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            vkDestroySurfaceKHR(Instance()->_vInstance, _vSurface, nullptr);
        }

        glfwDestroyWindow(_pWindow);
        _pWindow = nullptr;
    }

    bool Window::ShouldClose() {
        if (!Opened()) {
            return false;
        }

        return glfwWindowShouldClose(_pWindow);
    }

    void Window::LoopStep() {
        DrawFrame();
    }

    void Window::DrawFrame() {
        VkResult result = vkAcquireNextImageKHR(Instance()->_vDevice, _vSwapChain, UINT64_MAX
            , _vImageAvailableSemaphore[GetProperImageDataIndex()], VK_NULL_HANDLE, &_currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            Error("failed to acquire swap chain image!");
        }

        auto renderedSemaphored = Draw();

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore signalSemaphores[] = { renderedSemaphored };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { _vSwapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &_currentImageIndex;
        presentInfo.pResults = nullptr; // Optional

        result = vkQueuePresentKHR(Instance()->_vPresentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
            _framebufferResized = false;
            RecreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            Error("failed to present swap chain image!");
        }
    }

    size_t Window::GetProperImageDataIndex() const {
        return Instance()->_globalFrameCounter % MAX_FRAMES_IN_FLIGHT;
    }

}
