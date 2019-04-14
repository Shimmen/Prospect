#include "CameraBase.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>

#include "GuiSystem.h"

void
CameraBase::LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
{
	this->position = position;

	auto direction = glm::normalize(target - position);
	this->orientation = glm::quatLookAtLH(direction, up);
}

void
CameraBase::DrawEditorGui()
{
	// Aperture
	{
		const float apertureSteps[] = { 1.4f, 2.0f, 2.8f, 4.0f, 5.6f, 8.0f, 11.0f, 16.0f };
		const int apertureStepCount = sizeof(apertureSteps) / sizeof(apertureSteps[0]);

		ImGui::Spacing(); ImGui::Spacing();
		ImGui::Text("Aperture f/%.1f", aperture);
		GuiSystem::SnapSliderFloat("aperture", &aperture, apertureSteps, apertureStepCount, "");
	}

	// Shutter speed
	{
		const int shutterDenominators[] = { 1000, 500, 250, 125, 60, 30, 15, 8, 4, 2, 1 };
		const int shutterDenominatorCount = sizeof(shutterDenominators) / sizeof(shutterDenominators[0]);
		
		static int index = 1; // (given default f/16, ISO 400 we want ~1/400 s shutter speed)

		ImGui::Spacing(); ImGui::Spacing();
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

		ImGui::Spacing(); ImGui::Spacing();
		ImGui::Text("ISO %i", 100 * isoHundreds);
		ImGui::SliderInt("ISO", &isoHundreds, 1, 64, "");

		iso = float(isoHundreds * 100.0f);
	}
}