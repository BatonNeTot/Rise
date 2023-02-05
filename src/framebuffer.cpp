//☀Rise☀
#include "Rise/framebuffer.h"

#include "Rise/render_pass.h"
#include "Rise/image.h"
#include "Rise/rise.h"

namespace Rise {

	Framebuffer::Framebuffer(Core* core, const RenderPass& renderPass, IImage& image)
        : RiseObject(core), _renderPass(&renderPass), _image(&image) {
        VkImageView attachments[] = { image.vImageView() };

        auto& extent = image.GetMeta()->size;

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass.VRenderPass();
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(Instance()->_vDevice, &framebufferInfo, nullptr, &_vFramebuffer) != VK_SUCCESS) {
            Error("failed to create framebuffer!");
        }
	}

	Framebuffer::~Framebuffer() {
		vkDestroyFramebuffer(Instance()->_vDevice, _vFramebuffer, nullptr);
	}

    const VkExtent2D& Framebuffer::extent() const {
        return image()->GetMeta()->size;
    }

}
