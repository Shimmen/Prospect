#include "CameraBase.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <imgui.h>

#include "GuiSystem.h"
#include "PerformOnce.h"

#include "shader_locations.h"

void CameraBase::CommitToGpu(float deltaTimeREMOVEME_AND_PUT_IN_SOME_OTHER_BUFFER)
{
	PerformOnce(cameraBuffer.BindBufferBase(BufferObjectType::Uniform, PredefinedUniformBlockBinding(CameraUniformBlock)));

	// Save previous frame matrices before changing anything
	const mat4 prevMvp = cameraBuffer.memory.projection_from_view * cameraBuffer.memory.view_from_world;
	cameraBuffer.memory.prev_projection_from_world = prevMvp;

	cameraBuffer.memory.view_from_world = viewFromWorld;
	cameraBuffer.memory.world_from_view = inverse(viewFromWorld);

	cameraBuffer.memory.projection_from_view = projectionFromView;
	cameraBuffer.memory.view_from_projection = inverse(projectionFromView);

	float projA = zFar / (zFar - zNear);
	float projB = (-zFar * zNear) / (zFar - zNear);
	vec4 nearFar = vec4(zNear, zFar, projA, projB);
	cameraBuffer.memory.near_far = nearFar;

	cameraBuffer.memory.frustum_jitter = vec4(frustumJitterUv, 0.0f, 0.0f);

	cameraBuffer.memory.aperture = aperture;
	cameraBuffer.memory.shutter_speed = shutterSpeed;
	cameraBuffer.memory.iso = iso;
	cameraBuffer.memory.exposure_compensation = exposureComp;

	cameraBuffer.memory.adaption_rate = adaptionRate;
	cameraBuffer.memory.use_automatic_exposure = useAutomaticExposure;

	// NOTE: This will force way more updates than maybe required, due to the noisy nature
	// of a delta time signal. Maybe move to some other smaller uniform buffer?
	cameraBuffer.memory.delta_time = deltaTimeREMOVEME_AND_PUT_IN_SOME_OTHER_BUFFER;

	cameraBuffer.UpdateGpuBuffer();
}

void
CameraBase::LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
{
	this->position = position;

	auto direction = glm::normalize(target - position);
	this->orientation = glm::quatLookAtLH(direction, up);
}

void CameraBase::Resize(int width, int height)
{
	targetPixelsWidth = width;
	targetPixelsHeight = height;
}

void CameraBase::ApplyFrustumJitter(const glm::vec2& jitterPixels)
{
	glm::vec2 jitterUv = jitterPixels / glm::vec2(targetPixelsWidth, targetPixelsHeight);

	const float jitterScale = 0.70f; // TODO: Parameterize!
	jitterUv *= jitterScale;

	glm::mat4 offsetMatrix = glm::translate(glm::vec3(jitterUv, 0.0f));
	projectionFromView = offsetMatrix * projectionFromView;

	// Save jitter to be able to "unjitter" the signal for the velocity map
	frustumJitterUv = jitterUv - prevFrustumJitterUv;
	prevFrustumJitterUv = jitterUv;
}


void
CameraBase::DrawEditorGui()
{
	//ImGui::Checkbox("Use automatic exposure", &useAutomaticExposure);
	if (ImGui::RadioButton("Automatic exposure", useAutomaticExposure)) useAutomaticExposure = true;
	if (ImGui::RadioButton("Manual exposure", !useAutomaticExposure)) useAutomaticExposure = false;

	ImGui::Spacing();
	ImGui::Spacing();

	if (useAutomaticExposure)
	{
		// Adaption rate
		{
			ImGui::Text("Adaption rate", &adaptionRate);
			ImGui::SliderFloat("", &adaptionRate, 0.0001f, 2.0f, "%.4f", 5.0f);
		}

		// Exposure compensation
		{
			ImGui::Text("Exposure Compensation", &exposureComp);
			ImGui::SliderFloat("ECs", &exposureComp, -5.0f, +5.0f, "%.1f");
		}
	}
	else
	{
		// Aperture
		{
			const float apertureSteps[] = { 1.4f, 2.0f, 2.8f, 4.0f, 5.6f, 8.0f, 11.0f, 16.0f };
			const int apertureStepCount = sizeof(apertureSteps) / sizeof(apertureSteps[0]);

			ImGui::Text("Aperture f/%.1f", aperture);
			GuiSystem::SnapSliderFloat("aperture", &aperture, apertureSteps, apertureStepCount, "");
		}

		// Shutter speed
		{
			const int shutterDenominators[] = { 1000, 500, 250, 125, 60, 30, 15, 8, 4, 2, 1 };
			const int shutterDenominatorCount = sizeof(shutterDenominators) / sizeof(shutterDenominators[0]);

			static int index = 1; // (given default f/16, ISO 400 we want ~1/400 s shutter speed)

			ImGui::Text("Shutter speed  1/%i s", shutterDenominators[index]);
			ImGui::SliderInt("shutter", &index, 0, shutterDenominatorCount - 1, "");

			shutterSpeed = 1.0f / shutterDenominators[index];
		}

		// ISO
		{
			static int isoHundreds = 0;
			if (int(iso) / 100 != isoHundreds)
			{
				isoHundreds = int(iso) / 100;
			}

			ImGui::Text("ISO %i", 100 * isoHundreds);
			ImGui::SliderInt("ISO", &isoHundreds, 1, 64, "");

			iso = float(isoHundreds * 100.0f);
		}
	}
}
