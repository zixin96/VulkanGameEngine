#pragma once

namespace ZZX
{
	class ZCamera
	{
	public:
		void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
		void setPerspectiveProjection(float fovy, float aspect, float near, float far);
		void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
		void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
		void setViewYXZ(glm::vec3 position, glm::vec3 rotation);
		const glm::mat4& getProjection() const { return m_projectionMatrix; }
		const glm::mat4& getView() const { return m_viewMatrix; }
		const glm::mat4& getInverseView() const { return m_inverseViewMatrix; }
		const glm::vec3 getPosition() const { return glm::vec3(m_inverseViewMatrix[3]); }
	private:
		glm::mat4 m_projectionMatrix{1.f};
		glm::mat4 m_viewMatrix{1.f};
		glm::mat4 m_inverseViewMatrix{ 1.f };
	};
};
