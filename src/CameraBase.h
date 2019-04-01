#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Input;

// TODO: We might still want to expose glm to the global/prospect namespace, but not just like this.
using namespace glm;

class CameraBase
{
public:

	CameraBase() = default;
	virtual ~CameraBase() = default;

	void LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = { 0, 1, 0 });

	virtual void Resize(int width, int height) = 0;
	virtual void Update(const Input& input, float dt) = 0;

	virtual const mat4& GetViewMatrix() = 0;
	virtual const mat4& GetProjectionMatrix() = 0;
	mat4 GetViewProjectionMatrix() { return GetProjectionMatrix() * GetViewMatrix(); }

	void DrawEditorGui();

protected:

	vec3 position{};
	quat orientation{};
	float fieldOfView{ 3.141562f / 3.0f };

	// Default values according to the "sunny 16 rule" (https://en.wikipedia.org/wiki/Sunny_16_rule)
	float aperture     { 16.0f };
	float iso          { 400.0f };
	float shutterSpeed { 1.0f / iso };

};
