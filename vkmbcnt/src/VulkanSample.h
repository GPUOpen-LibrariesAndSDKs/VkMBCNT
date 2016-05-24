//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef AMD_VULKAN_SAMPLE_H_
#define AMD_VULKAN_SAMPLE_H_

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <memory>

namespace AMD
{
///////////////////////////////////////////////////////////////////////////////
class VulkanComputeSample
{
public:
    VulkanComputeSample(const VulkanComputeSample&) = delete;
    VulkanComputeSample& operator= (const VulkanComputeSample&) = delete;

    VulkanComputeSample();
    virtual ~VulkanComputeSample();

    void Run();
    struct ImportTable;

protected:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;

    std::unique_ptr<ImportTable> importTable_;

    int queueFamilyIndex_ = -1;

private:
    VkCommandPool commandPool_;
    VkCommandBuffer commandBuffer_;

#ifdef _DEBUG
    VkDebugReportCallbackEXT debugCallback_;
#endif
};
}   // namespace AMD

#endif
