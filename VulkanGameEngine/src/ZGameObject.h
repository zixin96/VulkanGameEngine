#pragma once
#include "ZModel.h"

namespace ZZX
{
	struct TransformComponent
	{
		// position offset
		glm::vec3 translation{0.f};
		glm::vec3 scale{1.f};
		glm::vec3 rotation{0.f};

		// Matrix corresponds to Translate * Ry * Rx * Rz * Scale
		// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
		// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		glm::mat4 mat4();
		glm::mat3 normalMatrix();
	};

	struct PointLightComponent
	{
		float lightIntensity = 1.0f;
	};

	class ZGameObject
	{
	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, ZGameObject>;

		static ZGameObject createGameObject()
		{
			static id_t currentId = 0;
			return ZGameObject{currentId++};
		}

		static ZGameObject makePointLight(float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.f));

		ZGameObject(const ZGameObject&) = delete;
		ZGameObject& operator=(const ZGameObject&) = delete;
		ZGameObject(ZGameObject&&) = default;
		ZGameObject& operator=(ZGameObject&&) = default;

		id_t getId() const { return id; }
		std::shared_ptr<ZModel> m_model{};
		glm::vec3 m_color{};
		TransformComponent m_transform{};

		std::unique_ptr<PointLightComponent> m_pointLight = nullptr;
	private:
		ZGameObject(id_t objId) : id{objId}
		{
		}

		id_t id;
	};
};
