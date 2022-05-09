#pragma once
#include "ZCamera.h"
#include "ZGameObject.h"

#include <vulkan/vulkan.h>

namespace ZZX
{
	struct FrameInfo
	{
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		ZCamera& camera;
		VkDescriptorSet globalDescriptorSet;
		ZGameObject::Map& gameObjects;
	};
}
