#include "allocator.hpp"

#include <utility>

#include "error.hpp"
#include "to_string.hpp"

namespace vkutils {
    Allocator::Allocator() noexcept = default;

    Allocator::~Allocator() {
        if (VK_NULL_HANDLE != allocator) {
            vmaDestroyAllocator(allocator);
        }
    }

    Allocator::Allocator(const VmaAllocator allocator) noexcept
        : allocator(allocator) {
    }

    Allocator::Allocator(Allocator&& other) noexcept
        : allocator(std::exchange(other.allocator, VK_NULL_HANDLE)) {
    }

    Allocator& Allocator::operator=(Allocator&& other) noexcept {
        std::swap(allocator, other.allocator);
        return *this;
    }
}

namespace vkutils {
    Allocator create_allocator(const VulkanContext& aContext) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(aContext.physicalDevice, &props);

        VmaVulkanFunctions functions{
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = vkGetDeviceProcAddr
        };

        const VmaAllocatorCreateInfo allocatorCreateInfo{
            .physicalDevice = aContext.physicalDevice,
            .device = aContext.device,
            .pVulkanFunctions = &functions,
            .instance = aContext.instance,
            .vulkanApiVersion = props.apiVersion
        };

        VmaAllocator allocator = VK_NULL_HANDLE;
        if (const auto res = vmaCreateAllocator(&allocatorCreateInfo, &allocator);
            VK_SUCCESS != res) {
            throw Error("Unable to create allocator\n"
                        "vmaCreateAllocator() returned %s", to_string(res).c_str()
            );
        }

        return Allocator(allocator);
    }
}
