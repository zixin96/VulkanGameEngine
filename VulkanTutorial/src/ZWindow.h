#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace ZZX
{
	class ZWindow
	{
	public:
		ZWindow(int w, int h, const std::string& name);
		~ZWindow();

		// delete copy ctor and assignment to avoid dangling pointer
		ZWindow(const ZWindow&) = delete;
		ZWindow& operator=(const ZWindow&) = delete;

		bool shouldClose() { return glfwWindowShouldClose(m_window); }
		VkExtent2D getExtent() { return {static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)}; }

		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
	private:
		void initWindow();

		const int m_width;
		const int m_height;
		std::string m_name;
		GLFWwindow* m_window;
	};
}
