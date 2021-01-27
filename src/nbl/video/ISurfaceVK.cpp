#include "nbl/video/surface/ISurfaceVK.h"

#include "nbl/video/CVulkanPhysicalDevice.h"

namespace nbl {
namespace video
{

bool ISurfaceVK::isSupported(const IPhysicalDevice* dev, uint32_t _queueIx) const
{
    auto vkphd = static_cast<const CVulkanPhysicalDevice*>(dev)->getInternalObject();
    VkBool32 supported;
    vkGetPhysicalDeviceSurfaceSupportKHR(vkphd, _queueIx, m_surface, &supported);

    return static_cast<bool>(supported);
}

}
}