#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "BufferObject.h"

// TODO: We might still want to expose glm to the global/prospect namespace, but not just like this.
using namespace glm;
#include "camera_uniforms.h"

class Input;

class CameraBase
{
public:

	CameraBase() = default;
	virtual ~CameraBase() = default;

	void DrawEditorGui();

	void CommitToGpu(float deltaTimeREMOVEME_AND_PUT_IN_SOME_OTHER_BUFFER);

	void LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = { 0, 1, 0 });

	void Resize(int width, int height);
	virtual void Update(const Input& input, float dt) = 0;

	void ApplyFrustumJitter(const glm::vec2& jitterPixels);

	const vec3& GetPosition() const { return position; }
	const quat& GetOrientation() const { return orientation; }

	const mat4& GetViewMatrix() const { return viewFromWorld; }
	const mat4& GetProjectionMatrix() const { return projectionFromView; }
	mat4 GetViewProjectionMatrix() const { return GetProjectionMatrix() * GetViewMatrix(); }

protected:

	vec3 position{};
	quat orientation{};
	float fieldOfView{ 3.141562f / 3.0f };

	vec2 frustumJitterUv;
	vec2 prevFrustumJitterUv;

	float zNear{ 0.25f };
	float zFar{ 250.0f };

	int targetPixelsWidth;
	int targetPixelsHeight;

	mat4 viewFromWorld;
	mat4 projectionFromView;

	// Default values according to the "sunny 16 rule" (https://en.wikipedia.org/wiki/Sunny_16_rule)
	float aperture     { 16.0f };
	float iso          { 400.0f };
	float shutterSpeed { 1.0f / iso };
	float exposureComp { 0.0f };

	float adaptionRate { 0.0018f };
	bool useAutomaticExposure { true };

	BufferObject<CameraUniforms> cameraBuffer;

};
