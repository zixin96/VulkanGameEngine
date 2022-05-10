#pragma once

#include "ZDevice.h"
#include "ZGameObject.h"
#include "ZWindow.h"
#include "ZRenderer.h"
#include "ZDescriptors.h"

// std
#include <memory>
#include <vector>

namespace ZZX
{
	class FirstApp
	{
	public:
		// Application-specific window's width/height
		// Modify these if you want to change the resolution of the window:
		static constexpr int WINDOW_WIDTH = 3000;
		static constexpr int WINDOW_HEIGHT = 1600;

		FirstApp();
		~FirstApp();

		// delete copy ctor and assignment to avoid dangling pointer
		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();
	private:
		void loadGameObjects();
		ZWindow m_zWindow{WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Engine"};
		ZDevice m_zDevice{m_zWindow};
		ZRenderer m_zRenderer{ m_zWindow, m_zDevice };

		// note: order of declarations matters
		std::unique_ptr<ZDescriptorPool> m_globalPool{};
		ZGameObject::Map m_gameObjects;
	};
}
