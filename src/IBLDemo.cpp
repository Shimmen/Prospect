#include "IBLDemo.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include <imgui.h>

#include "GuiSystem.h"
#include "ShaderSystem.h"
#include "TransformSystem.h"
#include "MaterialSystem.h"
#include "TextureSystem.h"
#include "ModelSystem.h"

#include "FpsCamera.h"
#include "GBuffer.h"
#include "LightBuffer.h"
#include "ShadowMap.h"

#include "Scene.h"
#include "BasicMaterial.h"
#include "CompleteMaterial.h"

#include "GeometryPass.h"
#include "ShadowPass.h"
#include "LightPass.h"
#include "IBLPass.h"
#include "SkyPass.h"
#include "FinalPass.h"

using namespace glm;
#include "shader_types.h"
#include "shader_locations.h"
#include "camera_uniforms.h"

///////////////////////////////////////////////////////////////////////////////
// Data

namespace
{
	Scene scene{};
	Model testQuad;

	GBuffer gBuffer;
	LightBuffer lightBuffer;

	GeometryPass geometryPass;
	IBLPass iblPass;
	SkyPass skyPass;
	FinalPass finalPass;
}

///////////////////////////////////////////////////////////////////////////////
// Application lifetime

App::Settings IBLDemo::Setup()
{
	Settings settings{};
	settings.window.size = { 1280, 800 };
	settings.window.resizeable = true;
	settings.context.msaaSamples = 1;
	return settings;
}

void IBLDemo::Init()
{
	ModelSystem::LoadModel("assets/generic/sphere.obj", [&](std::vector<Model> models) {
		const int gridSize = 10;

		assert(models.size() == 1);
		Model model = models[0];

		for (int y = 0; y < gridSize; ++y)
		{
			for (int x = 0; x < gridSize; ++x)
			{
				int id = TransformSystem::Create();
				TransformSystem::Get(id).SetPosition(2.5f * x, 2.5f * y, 0.0f);
				TransformSystem::UpdateMatrices(id);

				Model sphere = model;
				sphere.transformID = id;

				BasicMaterial *material = new BasicMaterial();
				MaterialSystem::ManageMaterial(material);

				material->baseColor = SrgbColor(1.0, 1.0, 1.0);
				material->roughness = (float)y / (float)(gridSize - 1);
				material->metallic = 1.0f - (float)x / (float)(gridSize - 1);

				sphere.material = material;
				scene.models.emplace_back(sphere);
			}
		}
	});

	ModelSystem::LoadModel("assets/cerberus/cerberus.obj", [&](std::vector<Model> models) {
		assert(models.size() == 1);
		Model model = models[0];

		int id = TransformSystem::Create();
		TransformSystem::Get(id)
			.SetPosition(35.0f, 10.0f, -10.0f)
			.SetScale(20.0f);
		TransformSystem::UpdateMatrices(id);
		model.transformID = id;

		scene.models.emplace_back(model);
	});

	scene.skyProbe.radiance = TextureSystem::LoadHdrImage("assets/env/rooftop_night/sky_2k.hdr");

	scene.mainCamera.LookAt({ 6, 11, -25 }, { 6, 11, 0 });
}

void IBLDemo::Resize(int width, int height)
{
	scene.mainCamera.Resize(width, height);

	gBuffer.RecreateGpuResources(width, height);
	lightBuffer.RecreateGpuResources(width, height, gBuffer);
}

///////////////////////////////////////////////////////////////////////////////
// Drawing / main loop

void IBLDemo::Draw(const Input& input, float deltaTime, float runningTime)
{
	ImGui::Begin("Prospect - IBL demo");
	if (input.WasKeyPressed(GLFW_KEY_HOME))
	{
		ImGui::SetWindowPos(ImVec2(0, 1));
		ImGui::SetWindowSize(ImVec2(350.0f, lightBuffer.height - 25.0f));
	}

	scene.mainCamera.Update(input, deltaTime);

	geometryPass.Draw(gBuffer, scene);
	iblPass.Draw(lightBuffer, gBuffer, scene);
	skyPass.Draw(lightBuffer, scene);
	finalPass.Draw(lightBuffer);

	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
