#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Input.h"

class FpsCamera
{
public:

	FpsCamera(/*const glm::vec3& position, const glm::vec3& lookDirection*/);
	~FpsCamera() = default;

	void Update(const Input& input, float dt);
	
	const glm::mat4& GetViewMatrix();
	const glm::mat4& GetProjectionMatrix();


private:

	glm::vec3 position{};
	glm::vec3 velocity{};
	
	const float maxSpeed = 10.0f;
	const float timeToMaxSpeed = 0.25f;
	const float timeFromMaxSpeed = 0.60f;
	const float stopTreshold = 0.01f;

	//

	glm::quat orientation{ 1, 0, 0, 0 };
	glm::vec3 pitchYawRoll{ 0.0f };

	float rotationMultiplier = 50.0f;
	float rotationDampening = 0.000005f;

	glm::quat bankingOrientation{ 1, 0, 0, 0 };
	const float baselineBankAngle{ glm::radians(30.0f) };

	//

	float fieldOfView{ glm::radians(60.0f) };
	float targetFieldOfView{ fieldOfView };

	const float zoomSensitivity{ 0.15f };
	const float minFieldOfView{ glm::radians(15.0f) };
	const float maxFieldOfView{ glm::radians(60.0f) };
	
	//
	
	glm::mat4 viewFromWorld;
	glm::mat4 projectionFromView;

};
