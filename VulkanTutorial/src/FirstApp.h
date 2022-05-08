#pragma once

#include "ZDevice.h"
#include "ZGameObject.h"
#include "ZWindow.h"
#include "ZRenderer.h"

// std
#include <memory>
#include <vector>

namespace ZZX
{
	class FirstApp
	{
	public:
		// App-specific width/height
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp();
		~FirstApp();

		// delete copy ctor and assignment to avoid dangling pointer
		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();
	private:
		void loadGameObjects();

		ZWindow m_zWindow{WIDTH, HEIGHT, "Vulkan Renderer"};
		ZDevice m_zDevice{m_zWindow};
		ZRenderer m_zRenderer{ m_zWindow, m_zDevice };
		std::vector<ZGameObject> m_gameObjects;
	};
}
