#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <glad/glad.h>

#include "Input.h"

#include "camera_uniforms.h"

class FpsCamera
{
public:

	FpsCamera() = default;
	~FpsCamera() = default;

	void LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = {0, 1, 0});

	void Resize(int width, int height);
	void Update(const Input& input, float dt);
	
	const glm::mat4& GetViewMatrix();
	const glm::mat4& GetProjectionMatrix();
	const glm::mat4 GetViewProjectionMatrix();

private:

	glm::vec3 position{};
	glm::vec3 velocity{};
	
	const float maxSpeed = 10.0f;
	const float timeToMaxSpeed = 0.25f;
	const float timeFromMaxSpeed = 0.60f;
	const float stopTreshold = 0.02f;

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
	
	float aspectRatio{ 16.0f / 9.0f };

	glm::mat4 viewFromWorld;
	glm::mat4 projectionFromView;

	//

	CameraUniforms cameraUniformData;
	GLuint cameraUniformBuffer;

};
