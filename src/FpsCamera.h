#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <glad/glad.h>

#include "Input.h"

#include "CameraBase.h"

#include "camera_uniforms.h"

class FpsCamera: public CameraBase
{
public:

	FpsCamera() = default;
	~FpsCamera() = default;

	void Resize(int width, int height) override;
	void Update(const Input& input, float dt) override;
	
	const mat4& GetViewMatrix() override;
	const mat4& GetProjectionMatrix() override;

private:

	vec3 velocity{};
	
	const float maxSpeed = 10.0f;
	const float timeToMaxSpeed = 0.25f;
	const float timeFromMaxSpeed = 0.60f;
	const float stopThreshold = 0.02f;

	//

	vec3 pitchYawRoll{ 0.0f };

	float rotationMultiplier = 50.0f;
	float rotationDampening = 0.000005f;

	quat bankingOrientation{ 1, 0, 0, 0 };
	const float baselineBankAngle{ radians(30.0f) };

	//

	float fieldOfView{ radians(60.0f) };
	float targetFieldOfView{ fieldOfView };

	const float zoomSensitivity{ 0.15f };
	const float minFieldOfView{ radians(15.0f) };
	const float maxFieldOfView{ radians(60.0f) };
	
	//

	// NOTE: This is just the default value
	float aspectRatio{ 16.0f / 9.0f };

	mat4 viewFromWorld;
	mat4 projectionFromView;

	//

	CameraUniforms cameraUniformData;
	GLuint cameraUniformBuffer;

};
