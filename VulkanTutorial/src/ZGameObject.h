#pragma once
#include "ZModel.h"

#include <memory>

namespace ZZX
{
	struct Transform2dComponent
	{
		// position offset
		glm::vec2 translation{};
		glm::vec2 scale{1.f, 1.f};
		float rotation;

		glm::mat2 mat2()
		{
			const float s = glm::sin(rotation);
			const float c = glm::cos(rotation);
			glm::mat2 rotMatrix{{c, s}, {-s, c}};
			glm::mat2 scaleMat{{scale.x, 0.0f}, {0.0f, scale.y}};
			return rotMatrix * scaleMat;
		}
	};

	class ZGameObject
	{
	public:
		using id_t = unsigned int;

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
		Transform2dComponent m_transform2d{};
	private:
		ZGameObject(id_t objId) : id{objId}
		{
		}

		id_t id;
	};
};
