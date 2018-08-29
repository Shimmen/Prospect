#include "FpsCamera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GuiSystem.h"

#include "shader_locations.h"

void
FpsCamera::LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
{
	this->position = position;

	auto direction = glm::normalize(target - position);
	this->orientation = glm::quatLookAtLH(direction, up);
}

void
FpsCamera::Resize(int width, int height)
{
	aspectRatio = float(width) / float(height);
}

void
FpsCamera::Update(const Input& input, float dt)
{
	// Apply acceleration from input

	glm::vec3 acceleration{ 0.0f };

	if (input.IsKeyDown(GLFW_KEY_W)) acceleration.z += 1;
	if (input.IsKeyDown(GLFW_KEY_S)) acceleration.z -= 1;

	if (input.IsKeyDown(GLFW_KEY_D)) acceleration.x += 1;
	if (input.IsKeyDown(GLFW_KEY_A)) acceleration.x -= 1;

	if (input.IsKeyDown(GLFW_KEY_SPACE))      acceleration.y += 1;
	if (input.IsKeyDown(GLFW_KEY_LEFT_SHIFT)) acceleration.y -= 1;

	if (glm::length2(acceleration) > 0.01f && !GuiSystem::IsUsingKeyboard())
	{
		acceleration = glm::normalize(acceleration) * (maxSpeed / timeToMaxSpeed) * dt;
		velocity += glm::rotate(orientation, acceleration);
	}
	else
	{
		// If no input and movement to acceleration deaccelerate insted
		if (glm::length2(velocity) < stopTreshold)
		{
			velocity = glm::vec3(0, 0, 0);
		}
		else
		{
			glm::vec3 deaccel = -glm::normalize(velocity) * (maxSpeed / timeFromMaxSpeed) * dt;
			velocity += deaccel;
		}
	}

	// Apply velocity to position

	float speed = glm::length(velocity);
	if (speed > 0.0f)
	{
		speed = glm::clamp(speed, 0.0f, maxSpeed);
		velocity = glm::normalize(velocity) * speed;

		position += velocity * dt;
	}

	// Calculate rotation velocity from input

	if (input.IsButtonDown(GLFW_MOUSE_BUTTON_2) && !GuiSystem::IsUsingMouse())
	{
		glm::vec2 screenSize = { 1280, 800 }; // TODO: Get from somewhere!

		// Screen size independent but also aspect ratio dependent!
		glm::vec2 mouseDelta = input.GetMouseDelta() / screenSize.x;

		// Make rotations less sensitive when zoomed in
		float fovMultiplier = 0.2f + ((fieldOfView - minFieldOfView) / (maxFieldOfView - minFieldOfView)) * 0.8f;

		pitchYawRoll.x += mouseDelta.x * rotationMultiplier * fovMultiplier * dt;
		pitchYawRoll.y += mouseDelta.y * rotationMultiplier * fovMultiplier * dt;
	}

	// Calculate banking due to movement

	glm::vec3 right = glm::rotate(orientation, glm::vec3(1, 0, 0));
	glm::vec3 forward = glm::rotate(orientation, glm::vec3(0, 0, 1));

	if (speed > 0.0f)
	{
		auto direction = velocity / speed;
		float speedAlongRight = glm::dot(direction, right) * speed;
		float signOrZeroSpeed = float(speedAlongRight > 0.0f) - float(speedAlongRight < 0.0f);
		float bankAmountSpeed = std::abs(speedAlongRight) / maxSpeed * 2.0f;

		float rotationAlongY = pitchYawRoll.x;
		float signOrZeroRotation = float(rotationAlongY > 0.0f) - float(rotationAlongY < 0.0f);
		float bankAmountRotation = glm::clamp(std::abs(rotationAlongY) * 100.0f, 0.0f, 3.0f);

		float targetBank = ((signOrZeroSpeed * -bankAmountSpeed) + (signOrZeroRotation * -bankAmountRotation)) * baselineBankAngle;
		pitchYawRoll.z = glm::mix(pitchYawRoll.z, targetBank, 1.0f - pow(0.35f, dt));
	}

	// Damp rotation continuously

	pitchYawRoll *= pow(rotationDampening, dt);

	// Apply rotation
	
	orientation = glm::angleAxis(pitchYawRoll.y, right) * orientation;
	orientation = glm::angleAxis(pitchYawRoll.x, glm::vec3(0, 1, 0)) * orientation;

	bankingOrientation = glm::angleAxis(pitchYawRoll.z, forward);

	// Apply zoom

	if (!GuiSystem::IsUsingMouse())
	{
		targetFieldOfView += -input.GetScrollDelta() * zoomSensitivity;
		targetFieldOfView = glm::clamp(targetFieldOfView, minFieldOfView, maxFieldOfView);
	}
	fieldOfView = glm::mix(fieldOfView, targetFieldOfView, 1.0f - pow(0.01f, dt));

	// Create the view matrix

	auto preAdjustedUp = glm::rotate(orientation, glm::vec3(0, 1, 0));
	auto up = glm::rotate(bankingOrientation, preAdjustedUp);

	glm::vec3 lookAt = position + forward;
	viewFromWorld = glm::lookAtLH(position, lookAt, up);

	// Create the projection matrix

	float cameraNear = 0.2f;
	float cameraFar = 1000.0f;
	projectionFromView = glm::perspectiveLH(fieldOfView, aspectRatio, cameraNear, cameraFar);

	// Update the camera uniform buffer

	if (!cameraUniformBuffer)
	{
		glCreateBuffers(1, &cameraUniformBuffer);
		glNamedBufferStorage(cameraUniformBuffer, sizeof(CameraUniforms), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glBindBufferBase(GL_UNIFORM_BUFFER, PredefinedUniformBlockBinding(CameraUniformBlock), cameraUniformBuffer);
	}

	bool performUpdate = false;

	if (cameraUniformData.view_from_world != viewFromWorld)
	{
		cameraUniformData.view_from_world = viewFromWorld;
		cameraUniformData.world_from_view = glm::inverse(viewFromWorld);
		performUpdate = true;
	}

	if (cameraUniformData.projection_from_view != projectionFromView)
	{
		cameraUniformData.projection_from_view = projectionFromView;
		cameraUniformData.view_from_projection = glm::inverse(projectionFromView);
		performUpdate = true;
	}

	glm::vec4 nearFar = glm::vec4(cameraNear, cameraFar, 0, 0);
	if (cameraUniformData.near_far != nearFar)
	{
		cameraUniformData.near_far = nearFar;
		performUpdate = true;
	}

	if (performUpdate)
	{
		glNamedBufferSubData(cameraUniformBuffer, 0, sizeof(CameraUniforms), &cameraUniformData);
	}

}

const glm::mat4&
FpsCamera::GetViewMatrix()
{
	return viewFromWorld;
}

const glm::mat4&
FpsCamera::GetProjectionMatrix()
{
	return projectionFromView;
}
