//☀Rise☀
#ifndef framebuffer_h
#define framebuffer_h

#include "rise_object.h"
#include "resource.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <unordered_map>

namespace Rise {

    class RenderPass;
    class IImage;

    class Framebuffer : public RiseObject {
    public:

        Framebuffer(Core* core, const RenderPass& renderPass, IImage& image);
        ~Framebuffer();

        const RenderPass* renderPass() {
            return _renderPass;
        }

        const IImage* image() const {
            return _image;
        }

        const VkExtent2D& extent() const;

        VkFramebuffer VFramebuffer() {
            return _vFramebuffer;
        }

    private:

        const RenderPass* _renderPass; 
        IImage* _image;
        VkFramebuffer _vFramebuffer;
    };

}

#endif /* framebuffer_h */
