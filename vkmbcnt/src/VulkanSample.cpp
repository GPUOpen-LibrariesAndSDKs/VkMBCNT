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

#include "VulkanSample.h"

#include <iostream>
#include <algorithm>
#include <set>
#include <string>

#include "Utility.h"

#include "Shaders.h"

#include <cassert>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#pragma warning( disable : 4100 ) // disable unreferenced formal parameter warnings

namespace AMD
{
struct VulkanComputeSample::ImportTable
{
#define GET_INSTANCE_ENTRYPOINT(i, w) w = reinterpret_cast<PFN_##w>(vkGetInstanceProcAddr(i, #w))
#define GET_DEVICE_ENTRYPOINT(i, w) w = reinterpret_cast<PFN_##w>(vkGetDeviceProcAddr(i, #w))

    ImportTable() = default;

    ImportTable(VkInstance instance, VkDevice /*device*/)
    {
#ifdef _DEBUG
        GET_INSTANCE_ENTRYPOINT(instance, vkCreateDebugReportCallbackEXT);
        GET_INSTANCE_ENTRYPOINT(instance, vkDebugReportMessageEXT);
        GET_INSTANCE_ENTRYPOINT(instance, vkDestroyDebugReportCallbackEXT);
#endif
    }

#undef GET_INSTANCE_ENTRYPOINT
#undef GET_DEVICE_ENTRYPOINT

#ifdef _DEBUG
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = nullptr;
    PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT = nullptr;
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = nullptr;
#endif
};

namespace
{
///////////////////////////////////////////////////////////////////////////////
VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
    VkDebugReportFlagsEXT       /*flags*/,
    VkDebugReportObjectTypeEXT  /*objectType*/,
    uint64_t                    /*object*/,
    size_t                      /*location*/,
    int32_t                     /*messageCode*/,
    const char*                 /*pLayerPrefix*/,
    const char*                 pMessage,
    void*                       /*pUserData*/)
{
    OutputDebugStringA(pMessage);
    OutputDebugStringA("\n");
    return VK_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
std::set<std::string> GetDeviceExtensions(VkPhysicalDevice device)
{
    uint32_t extensionCount = 0;

    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
        nullptr);

    std::vector<VkExtensionProperties> deviceExtensions{ extensionCount };

    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
        deviceExtensions.data());

    std::set<std::string> result;

    for (const auto& e : deviceExtensions)
    {
        result.insert(e.extensionName);
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
std::vector<const char*> GetDebugInstanceLayerNames()
{
    uint32_t layerCount = 0;

    vkEnumerateInstanceLayerProperties(&layerCount,
        nullptr);

    std::vector<VkLayerProperties> instanceLayers{ layerCount };

    vkEnumerateInstanceLayerProperties(&layerCount,
        instanceLayers.data());

    std::vector<const char*> result;
    for (const auto& p : instanceLayers)
    {
        if (strcmp(p.layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
        {
            result.push_back("VK_LAYER_LUNARG_standard_validation");
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
std::vector<const char*> GetDebugInstanceExtensionNames()
{
    uint32_t extensionCount = 0;

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
        nullptr);

    std::vector<VkExtensionProperties> instanceExtensions{ extensionCount };

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
        instanceExtensions.data());

    std::vector<const char*> result;
    for (const auto& e : instanceExtensions)
    {
        if (strcmp(e.extensionName, "VK_EXT_debug_report") == 0)
        {
            result.push_back("VK_EXT_debug_report");
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
std::vector<const char*> GetDebugDeviceLayerNames(VkPhysicalDevice device)
{
    uint32_t layerCount = 0;
    vkEnumerateDeviceLayerProperties(device, &layerCount, nullptr);

    std::vector<VkLayerProperties> deviceLayers{ layerCount };
    vkEnumerateDeviceLayerProperties(device, &layerCount, deviceLayers.data());

    std::vector<const char*> result;
    for (const auto& p : deviceLayers)
    {
        if (strcmp(p.layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
        {
            result.push_back("VK_LAYER_LUNARG_standard_validation");
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
void FindPhysicalDeviceWithComputeQueue(const std::vector<VkPhysicalDevice>& physicalDevices,
    VkPhysicalDevice* outputDevice, int* outputComputeQueueIndex)
{
    for (auto physicalDevice : physicalDevices)
    {
        uint32_t queueFamilyPropertyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
            &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties{ queueFamilyPropertyCount };
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
            &queueFamilyPropertyCount, queueFamilyProperties.data());

        int i = 0;
        for (const auto& queueFamilyProperty : queueFamilyProperties)
        {
            if (queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT
                || queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                if (outputDevice)
                {
                    *outputDevice = physicalDevice;
                }

                if (outputComputeQueueIndex)
                {
                    *outputComputeQueueIndex = i;
                }

                return;
            }

            ++i;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
VkInstance CreateInstance()
{
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    std::vector<const char*> instanceExtensions;

#ifdef _DEBUG
    auto debugInstanceExtensionNames = GetDebugInstanceExtensionNames();
    instanceExtensions.insert(instanceExtensions.end(),
        debugInstanceExtensionNames.begin(), debugInstanceExtensionNames.end());
#endif

    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t> (instanceExtensions.size());

    std::vector<const char*> instanceLayers;

#ifdef _DEBUG
    auto debugInstanceLayerNames = GetDebugInstanceLayerNames();
    instanceLayers.insert(instanceLayers.end(),
        debugInstanceLayerNames.begin(), debugInstanceLayerNames.end());
#endif

    instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t> (instanceLayers.size());

    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = VK_API_VERSION_1_0;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pApplicationName = "AMD Vulkan Compute Sample application";
    applicationInfo.pEngineName = "AMD Vulkan Compute Sample Engine";

    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    VkInstance instance = nullptr;
    vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

    return instance;
}

///////////////////////////////////////////////////////////////////////////////
void CreateDeviceAndQueue(VkInstance instance, VkDevice* outputDevice,
    VkQueue* outputQueue, int* outputQueueIndex,
    VkPhysicalDevice* outputPhysicalDevice)
{
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

    std::vector<VkPhysicalDevice> devices{ physicalDeviceCount };
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
        devices.data());

    VkPhysicalDevice physicalDevice = nullptr;
    int graphicsQueueIndex = -1;

    FindPhysicalDeviceWithComputeQueue(devices, &physicalDevice, &graphicsQueueIndex);

    assert(physicalDevice);

    // Check if the device supports the SPIR-V extensions
    const auto extensions = GetDeviceExtensions(physicalDevice);

    if (extensions.find("VK_AMD_shader_ballot") == extensions.end())
    {
        std::cerr << "AMD_shader_ballot is not supported" << std::endl;
        exit(1);
        return;
    }

    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueIndex;

    static const float queuePriorities[] = { 1.0f };
    deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;

    VkPhysicalDeviceFeatures enabledFeatures = {};
    enabledFeatures.shaderInt64 = true;

    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

    std::vector<const char*> deviceLayers;

#ifdef _DEBUG
    auto debugDeviceLayerNames = GetDebugDeviceLayerNames(physicalDevice);
    deviceLayers.insert(deviceLayers.end(),
        debugDeviceLayerNames.begin(), debugDeviceLayerNames.end());
#endif

    deviceCreateInfo.ppEnabledLayerNames = deviceLayers.data();
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t> (deviceLayers.size());

    std::vector<const char*> deviceExtensions;

    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t> (deviceExtensions.size());

    VkDevice device = nullptr;
    vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    assert(device);

    VkQueue queue = nullptr;
    vkGetDeviceQueue(device, graphicsQueueIndex, 0, &queue);
    assert(queue);

    if (outputQueue)
    {
        *outputQueue = queue;
    }

    if (outputDevice)
    {
        *outputDevice = device;
    }

    if (outputQueueIndex)
    {
        *outputQueueIndex = graphicsQueueIndex;
    }

    if (outputPhysicalDevice)
    {
        *outputPhysicalDevice = physicalDevice;
    }
}

#ifdef _DEBUG
///////////////////////////////////////////////////////////////////////////////
VkDebugReportCallbackEXT SetupDebugCallback(VkInstance instance, VulkanComputeSample::ImportTable* importTable)
{
    if (importTable->vkCreateDebugReportCallbackEXT)
    {
        VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
        callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        callbackCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT |
            VK_DEBUG_REPORT_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        callbackCreateInfo.pfnCallback = &DebugReportCallback;

        VkDebugReportCallbackEXT callback;
        importTable->vkCreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, &callback);
        return callback;
    }
    else
    {
        return VK_NULL_HANDLE;
    }
}

///////////////////////////////////////////////////////////////////////////////
void CleanupDebugCallback(VkInstance instance, VkDebugReportCallbackEXT callback,
    VulkanComputeSample::ImportTable* importTable)
{
    if (importTable->vkDestroyDebugReportCallbackEXT)
    {
        importTable->vkDestroyDebugReportCallbackEXT(instance, callback, nullptr);
    }
}
#endif

struct MemoryTypeInfo
{
    bool deviceLocal = false;
    bool hostVisible = false;
    bool hostCoherent = false;
    bool hostCached = false;
    bool lazilyAllocated = false;

    struct Heap
    {
        uint64_t size = 0;
        bool deviceLocal = false;
    };

    Heap heap;
    int index;
};

///////////////////////////////////////////////////////////////////////////////
std::vector<MemoryTypeInfo> EnumerateHeaps(VkPhysicalDevice device)
{
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

    std::vector<MemoryTypeInfo::Heap> heaps;

    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i)
    {
        MemoryTypeInfo::Heap info;
        info.size = memoryProperties.memoryHeaps[i].size;
        info.deviceLocal = (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0;

        heaps.push_back(info);
    }

    std::vector<MemoryTypeInfo> result;

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        MemoryTypeInfo typeInfo;

        typeInfo.deviceLocal = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0;
        typeInfo.hostVisible = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
        typeInfo.hostCoherent = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
        typeInfo.hostCached = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0;
        typeInfo.lazilyAllocated = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) != 0;

        typeInfo.heap = heaps[memoryProperties.memoryTypes[i].heapIndex];

        typeInfo.index = static_cast<int> (i);

        result.push_back(typeInfo);
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
VkDeviceMemory AllocateMemory(const std::vector<MemoryTypeInfo>& memoryInfos,
    VkDevice device, const int size)
{
    // We take the first HOST_VISIBLE memory
    for (auto& memoryInfo : memoryInfos)
    {
        if (memoryInfo.hostVisible)
        {
            VkMemoryAllocateInfo memoryAllocateInfo = {};
            memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocateInfo.memoryTypeIndex = memoryInfo.index;
            memoryAllocateInfo.allocationSize = size;

            VkDeviceMemory deviceMemory;
            vkAllocateMemory(device, &memoryAllocateInfo, nullptr,
                &deviceMemory);
            return deviceMemory;
        }
    }

    return VK_NULL_HANDLE;
}

///////////////////////////////////////////////////////////////////////////////
VkShaderModule LoadShader(VkDevice device, const void* shaderContents,
    const size_t size)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    shaderModuleCreateInfo.pCode = static_cast<const uint32_t*> (shaderContents);
    shaderModuleCreateInfo.codeSize = size;

    VkShaderModule result;
    vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &result);

    return result;
}
}   // namespace

///////////////////////////////////////////////////////////////////////////////
VulkanComputeSample::VulkanComputeSample()
{
    instance_ = CreateInstance();

    VkPhysicalDevice physicalDevice;
    CreateDeviceAndQueue(instance_, &device_, &queue_, &queueFamilyIndex_,
        &physicalDevice);
    physicalDevice_ = physicalDevice;

    importTable_.reset(new ImportTable{ instance_, device_ });

#ifdef _DEBUG
    debugCallback_ = SetupDebugCallback(instance_, importTable_.get());
#endif

    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex_;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(device_, &commandPoolCreateInfo, nullptr,
        &commandPool_);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.commandPool = commandPool_;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device_, &commandBufferAllocateInfo,
        &commandBuffer_);
}

///////////////////////////////////////////////////////////////////////////////
VulkanComputeSample::~VulkanComputeSample()
{
    vkDestroyCommandPool(device_, commandPool_, nullptr);

#ifdef _DEBUG
    CleanupDebugCallback(instance_, debugCallback_, importTable_.get());
#endif

    vkDestroyDevice(device_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
void VulkanComputeSample::Run()
{
    static const int ElementCount = 64; // Number of elements we're going to
                                        // process in the kernel

    VkComputePipelineCreateInfo computePipelineCreateInfo = {};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

    computePipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computePipelineCreateInfo.stage.pName = "main";
    computePipelineCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    computePipelineCreateInfo.stage.module = LoadShader(device_, BasicComputeShader, sizeof(BasicComputeShader));

    VkBuffer inputBuffer, outputBuffer;
    VkBufferCreateInfo inputBufferCreateInfo = {};
    inputBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    inputBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    inputBufferCreateInfo.size = ElementCount * sizeof(float);

    vkCreateBuffer(device_, &inputBufferCreateInfo, nullptr, &inputBuffer);

    VkBufferCreateInfo outputBufferCreateInfo = {};
    outputBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    outputBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    outputBufferCreateInfo.size = ElementCount * sizeof(float);

    vkCreateBuffer(device_, &outputBufferCreateInfo, nullptr, &outputBuffer);

    VkMemoryRequirements inputBufferRequirements, outputBufferRequirements;
    vkGetBufferMemoryRequirements(device_, inputBuffer, &inputBufferRequirements);
    vkGetBufferMemoryRequirements(device_, outputBuffer, &outputBufferRequirements);

    VkDeviceSize bufferSize = inputBufferRequirements.size;
    const VkDeviceSize outputBufferOffset = RoundToNextMultiple(bufferSize,
        outputBufferRequirements.alignment);

    bufferSize = outputBufferOffset + outputBufferRequirements.size;

    auto memory = AllocateMemory(EnumerateHeaps(physicalDevice_), device_,
        static_cast<int> (bufferSize));

    void* mapping = nullptr;
    vkMapMemory(device_, memory, 0, VK_WHOLE_SIZE, 0, &mapping);

    float* data = static_cast<float*> (mapping);
    for (int i = 0; i < ElementCount; ++i)
    {
        // Input buffer is initialized to positive/negative numbers
        data[i] = ((i & 1) == 1) ? static_cast<float> (i) : static_cast<float>(-i);

        // Output buffer is initialized to 0
        data[i + outputBufferOffset / sizeof(float)] = 0;
    }

    vkUnmapMemory(device_, memory);

    vkBindBufferMemory(device_, inputBuffer, memory, 0);
    vkBindBufferMemory(device_, outputBuffer, memory, ElementCount * sizeof(float));

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[2] = {};
    descriptorSetLayoutBinding[0].binding = 0;
    descriptorSetLayoutBinding[0].descriptorCount = 1;
    descriptorSetLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorSetLayoutBinding[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    descriptorSetLayoutBinding[1].binding = 1;
    descriptorSetLayoutBinding[1].descriptorCount = 1;
    descriptorSetLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorSetLayoutBinding[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo[1] = {};
    descriptorSetLayoutCreateInfo[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo[0].bindingCount = 2;
    descriptorSetLayoutCreateInfo[0].pBindings = descriptorSetLayoutBinding;

    VkDescriptorSetLayout descriptorSetLayout[1];

    vkCreateDescriptorSetLayout(
        device_, descriptorSetLayoutCreateInfo,
        nullptr, descriptorSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 1;

    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo,
        nullptr, &pipelineLayout);

    computePipelineCreateInfo.layout = pipelineLayout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &computePipelineCreateInfo,
        nullptr, &pipeline);

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = 1;

    VkDescriptorPoolSize descriptorPoolSize = {};
    descriptorPoolSize.descriptorCount = 2;
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;

    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(device_, &descriptorPoolCreateInfo,
        nullptr, &descriptorPool);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayout;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;

    VkDescriptorSet descriptorSet;
    vkAllocateDescriptorSets(device_, &descriptorSetAllocateInfo, &descriptorSet);

    VkWriteDescriptorSet writeDescriptorSets[2] = {};
    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstSet = descriptorSet;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[0].dstBinding = 0;

    writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[1].dstSet = descriptorSet;
    writeDescriptorSets[1].descriptorCount = 1;
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[1].dstBinding = 1;

    VkDescriptorBufferInfo descriptorBufferInfo[2] = {};
    descriptorBufferInfo[0].buffer = inputBuffer;
    descriptorBufferInfo[0].offset = 0;
    descriptorBufferInfo[0].range = ElementCount * sizeof(float);
    descriptorBufferInfo[1].buffer = outputBuffer;
    descriptorBufferInfo[1].offset = 0;
    descriptorBufferInfo[1].range = ElementCount * sizeof(float);

    writeDescriptorSets[0].pBufferInfo = &descriptorBufferInfo[0];
    writeDescriptorSets[1].pBufferInfo = &descriptorBufferInfo[1];

    vkUpdateDescriptorSets(device_, 2, writeDescriptorSets, 0, nullptr);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer_, &commandBufferBeginInfo);
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDispatch(commandBuffer_, 1, 1, 1);

    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT, 0,
        1, &memoryBarrier, 0, nullptr, 0, nullptr);
    vkEndCommandBuffer(commandBuffer_);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer_;

    vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue_);
    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = memory;
    memoryRange.offset = 0;
    memoryRange.size = VK_WHOLE_SIZE;

    vkInvalidateMappedMemoryRanges(device_, 1, &memoryRange);

    // Output buffer is located at offset ElementCount * sizeof (float)
    vkMapMemory(device_, memory, outputBufferOffset,
        ElementCount * sizeof(float), 0, &mapping);
    data = static_cast<float*> (mapping);
    for (int i = 0; i < ElementCount; ++i)
    {
        std::cout << i << " : " << data[i] << "\n";
    }
    vkUnmapMemory(device_, memory);

    vkDestroyDescriptorSetLayout(device_, descriptorSetLayout[0], nullptr);
    vkDestroyDescriptorPool(device_, descriptorPool, nullptr);
    vkDestroyPipeline(device_, pipeline, nullptr);
    vkDestroyPipelineLayout(device_, pipelineLayout, nullptr);
    vkDestroyShaderModule(device_, computePipelineCreateInfo.stage.module, nullptr);
    vkDestroyBuffer(device_, inputBuffer, nullptr);
    vkDestroyBuffer(device_, outputBuffer, nullptr);
    vkFreeMemory(device_, memory, nullptr);
}

}   // namespace AMD
