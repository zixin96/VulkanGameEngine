#include "ZWindow.h"
#include <stdexcept>

namespace ZZX
{
	ZWindow::ZWindow(int w, int h, const std::string& name)
		: m_width(w), m_height(h), m_name(name)
	{
		initWindow();
	}

	ZWindow::~ZWindow()
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void ZWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
	{
		if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void ZWindow::initWindow()
	{
		// initialize the GLFW library
		glfwInit();
		// do not create an OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// create the actual window.
		m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
		/*glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);*/
	}
}
