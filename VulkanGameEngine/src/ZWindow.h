#pragma once

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
		bool wasWindowResized() { return m_framebufferResized; }
		void resetWindowResizedFlag() { m_framebufferResized = false; }
		GLFWwindow* getGLFWWindow() const { return m_window; }
		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
	private:
		void initWindow();

		// this function will be called when the window is resized by the user
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

		// window's dimension (changeable)
		int m_width;
		int m_height;

		// a flag to signal the window size is changed
		bool m_framebufferResized = false;

		std::string m_name;

		GLFWwindow* m_window;
	};
}
