#pragma once

#include <glm/glm.hpp>

#include "Input.h"

#include "CameraBase.h"

class FpsCamera: public CameraBase
{
public:

	FpsCamera() = default;
	~FpsCamera() = default;

	void Update(const Input& input, float dt) override;

private:

	vec3 velocity{};
	
	const float maxSpeed = 10.0f;
	const float timeToMaxSpeed = 0.25f;
	const float timeFromMaxSpeed = 0.60f;
	const float stopThreshold = 0.02f;

	//

	vec3 pitchYawRoll{ 0.0f };

	float rotationMultiplier = 30.0f;
	float rotationDampening = 0.000005f;

	quat bankingOrientation{ 1, 0, 0, 0 };
	const float baselineBankAngle{ radians(30.0f) };

	//

	float fieldOfView{ radians(60.0f) };
	float targetFieldOfView{ fieldOfView };

	const float zoomSensitivity{ 0.15f };
	const float minFieldOfView{ radians(15.0f) };
	const float maxFieldOfView{ radians(60.0f) };

};
