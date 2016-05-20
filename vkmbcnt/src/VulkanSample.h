#ifndef ANTERU_D3D12_SAMPLE_D3D12SAMPLE_H_
#define ANTERU_D3D12_SAMPLE_D3D12SAMPLE_H_

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <memory>

namespace AMD {

///////////////////////////////////////////////////////////////////////////////
class VulkanComputeSample
{
public:
	VulkanComputeSample (const VulkanComputeSample&) = delete;
	VulkanComputeSample& operator= (const VulkanComputeSample&) = delete;

	VulkanComputeSample ();
	virtual ~VulkanComputeSample ();

	void Run ();
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
}

#endif