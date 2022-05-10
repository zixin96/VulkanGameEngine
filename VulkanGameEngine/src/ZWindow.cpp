#include "pch.h"
#include "ZWindow.h"

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

	void ZWindow::initWindow()
	{
		// initialize the GLFW library
		glfwInit();

		// since GLFW is originally designed to create an OpenGL context, we need to tell it to not create an OpenGL context:
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);

		// listen to size changes on the glfwWindow
		glfwSetWindowUserPointer(m_window, this);
		glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
	}

	void ZWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
	{
		if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void ZWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		// update the dimensions of the window inside our wrapper class
		auto zWindow = reinterpret_cast<ZWindow*>(glfwGetWindowUserPointer(window));
		zWindow->m_framebufferResized = true;
		zWindow->m_width = width;
		zWindow->m_height = height;
	}
}
