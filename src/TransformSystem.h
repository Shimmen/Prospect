#pragma once

#include <array>
#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#ifndef MAX_NUM_TRANSFORMS
 #define MAX_NUM_TRANSFORMS 1024 * 1024
#endif

struct Transform
{
	Transform() = default;
	~Transform() = default;

	glm::vec3 position{};
	glm::quat orientation{ 1.0f, 0.0f, 0.0f, 0.0f};
	glm::vec3 scale{ 1.0f };

	glm::mat4 matrix{ 1.0f };
	glm::mat4 inverseMatrix{ 1.0f };
	glm::mat3 normalMatrix{ 1.0f };

	//

	Transform& SetPosition(float x, float y, float z)
	{
		position.x = x;
		position.y = y;
		position.z = z;
		return *this;
	}

	Transform& SetScale(float s)
	{
		SetScale(s, s, s);
		return *this;
	}

	Transform& SetScale(float x, float y, float z)
	{
		scale.x = x;
		scale.y = y;
		scale.z = z;
		return *this;
	}

	Transform& SetDirection(float x, float y, float z)
	{
		// From: https://gamedev.stackexchange.com/questions/149006/direction-vector-to-quaternion

		// Switch x and z if 90° off!
		float halfAngle = atan2(x, z) / 2.0f;

		orientation.x = 0.0f;
		orientation.y = sin(halfAngle);
		orientation.z = 0.0f;
		orientation.w = cos(halfAngle);

		return *this;
	}
};

namespace TransformSystem
{

	void Init();

	// Must be called before every frame
	void Update();

	int Create();
	
	Transform& Get(int transformID);
	const Transform& GetPrevious(int transformID);

	void UpdateMatrices(int transformID);

};
