#pragma once
#include "ZModel.h"

// libs
#include <glm/gtc/matrix_transform.hpp>

// std
#include <memory>
#include <unordered_map>

namespace ZZX
{
	struct TransformComponent
	{
		// position offset
		glm::vec3 translation{0.f};
		glm::vec3 scale{1.f};
		glm::vec3 rotation{0.f};

		// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
		// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
		// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		glm::mat4 mat4();
		glm::mat3 normalMatrix();
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

		ZGameObject(const ZGameObject&) = delete;
		ZGameObject& operator=(const ZGameObject&) = delete;
		ZGameObject(ZGameObject&&) = default;
		ZGameObject& operator=(ZGameObject&&) = default;

		id_t getId() const { return id; }
		std::shared_ptr<ZModel> m_model{};
		glm::vec3 m_color{};
		TransformComponent m_transform{};
	private:
		ZGameObject(id_t objId) : id{objId}
		{
		}

		id_t id;
	};
};
