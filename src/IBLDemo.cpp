#include "IBLDemo.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include <imgui.h>

#include "GuiSystem.h"
#include "TransformSystem.h"
#include "MaterialSystem.h"
#include "TextureSystem.h"
#include "ModelSystem.h"

#include "FpsCamera.h"
#include "GBuffer.h"

#include "Scene.h"
#include "BasicMaterial.h"
#include "RenderPipeline.h"

using namespace glm;

///////////////////////////////////////////////////////////////////////////////
// Util

float RandomFloat(float rmin, float rmax)
{
	float rand01 = float(rand()) / float(RAND_MAX);
	return rmin + rand01 * (rmax - rmin);
}

///////////////////////////////////////////////////////////////////////////////
// Data

struct BouncingModel
{
	int transformId;
	float sineScale;
	float sineAmplitude;
	glm::vec3 basePosition;
	glm::vec3 bounceDirection;
};

namespace
{
	Scene scene{};
	std::vector<BouncingModel> bouncingModels{};

	int screenWidth;
	int screenHeight;

	RenderPipeline renderPipeline;
}

///////////////////////////////////////////////////////////////////////////////
// Application lifetime

App::Settings IBLDemo::Setup()
{
	Settings settings{};
	settings.window.fullscreen = false;
	settings.window.size = { 1920, 1080 };
	settings.window.vsync = false;
	settings.window.resizeable = true;
	settings.context.msaaSamples = 0;
	return settings;
}

void IBLDemo::Init()
{
	ModelSystem::LoadModel("assets/generic/sphere.obj", [&](std::vector<Model> models) {

		assert(models.size() == 1);
		Model model = models[0];

		//
		// Material gallery:
		//
		{
			const int numMetalLevels = 2;
			const int numRoughnessLevels = 10;

			for (int y = 0; y < numMetalLevels; ++y)
			{
				for (int x = 0; x < numRoughnessLevels; ++x)
				{
					int id = TransformSystem::Create();
					TransformSystem::Get(id).SetPosition(3.6f * x, 8.0f + 3.6f * y, 0.0f);
					TransformSystem::UpdateMatrices(id);

					Model sphere = model;
					sphere.transformID = id;

					BasicMaterial *material = new BasicMaterial();
					MaterialSystem::ManageMaterial(material);

					material->baseColor = SrgbColor(0.972f, 0.960f, 0.915f);
					material->roughness = float(x) / float(numRoughnessLevels - 1);
					material->metallic = 1.0f - float(y) / float(numMetalLevels - 1);

					sphere.material = material;
					scene.models.emplace_back(sphere);
				}
			}
		}

		//
		// Bouncing spheres:
		//
		std::srand(1234);
		for (int i = 0; i < 50; ++i)
		{
			int id = TransformSystem::Create();

			// Generate random position in cube
			glm::vec3 position = {
				RandomFloat(-50.0f, -15.0f),
				RandomFloat(6.0f, 16.0f),
				RandomFloat(-50.0f, -15.0f)
			};

			TransformSystem::Get(id).position = position;
			TransformSystem::UpdateMatrices(id);

			Model sphere = model;
			sphere.transformID = id;

			auto *material = new BasicMaterial();
			MaterialSystem::ManageMaterial(material);

			material->baseColor = SrgbColor(
				RandomFloat(0.5f, 1.0f),
				RandomFloat(0.5f, 1.0f),
				RandomFloat(0.5f, 1.0f)
			);
			material->roughness = 0.15f;// RandomFloat(0.2f, 0.8f);
			material->metallic = 0.0f;// float(rand() % 2);

			sphere.material = material;
			scene.models.emplace_back(sphere);

			// Generate random direction to "bounce" in
			glm::vec3 bounceDirection = {
				RandomFloat(-1.0f, +1.0f),
				RandomFloat(-1.0f, +1.0f),
				RandomFloat(-1.0f, +1.0f)
			};
			bounceDirection = glm::normalize(bounceDirection);

			BouncingModel bm;
			bm.transformId = id;
			bm.basePosition = position;
			bm.bounceDirection = bounceDirection;
			bm.sineScale = RandomFloat(1.5f, 6.0f);
			bm.sineAmplitude = RandomFloat(3.0f, 10.0f);
			bouncingModels.emplace_back(bm);
		}
	});

	ModelSystem::LoadModel("assets/cerberus/cerberus.obj", [&](std::vector<Model> models) {
		assert(models.size() == 1);
		Model model = models[0];

		int id = TransformSystem::Create();
		TransformSystem::Get(id)
			.SetPosition(38.0f, 10.0f, -10.0f)
			.SetScale(20.0f);
		TransformSystem::UpdateMatrices(id);
		model.transformID = id;

		scene.models.emplace_back(model);
	});

	ModelSystem::LoadModel("assets/test_room/test_room.obj", [&](std::vector<Model> models)
	{
		for (Model& model : models)
		{
			int id = TransformSystem::Create();
			TransformSystem::Get(id)
				.SetPosition(18.0f, 12.0f, 25.0f)
				.SetDirection(5, 0, 2)
				.SetScale(2.8f);
			TransformSystem::UpdateMatrices(id);
			model.transformID = id;

			scene.models.emplace_back(model);
		}
	});

	DirectionalLight sun;
	sun.worldDirection = glm::normalize(glm::vec4(-1.0f, -0.5f, +1.85f, 0.0f));
	sun.color = Color(1, 1, 1, 0.55f);
	sun.softness.x = 2.3f;
	scene.directionalLights.push_back(sun);

	//scene.skyProbe.radiance = TextureSystem::LoadHdrImage("assets/env/rooftop_night/sky_2k.hdr");
	scene.skyProbe.radiance = TextureSystem::LoadHdrImage("assets/env/aero_lab/aerodynamics_workshop_8k.hdr");
	//scene.skyProbe.radiance = TextureSystem::CreatePlaceholder(255, 255, 255);

	scene.mainCamera.reset(new FpsCamera());
	scene.mainCamera->LookAt({ 6, 11, -25 }, { 6, 11, 0 });
}

void IBLDemo::Resize(int width, int height)
{
	screenWidth = width;
	screenHeight = height;

	scene.mainCamera->Resize(width, height);
	renderPipeline.Resize(width, height);
}

///////////////////////////////////////////////////////////////////////////////
// Drawing / main loop

void IBLDemo::Draw(const Input& input, float deltaTime, float runningTime)
{
	ImGui::Begin("Prospect - IBL demo");
	ImGui::Text("Frame time: %.1f ms", deltaTime * 1000);
	if (input.WasKeyPressed(GLFW_KEY_HOME))
	{
		ImGui::SetWindowPos(ImVec2(0, 1));
		ImGui::SetWindowSize(ImVec2(350.0f, screenHeight - 25.0f));
	}

	for (auto& bm : bouncingModels)
	{
		glm::vec3 pos = bm.basePosition + bm.bounceDirection * std::sinf(runningTime * bm.sineScale) * bm.sineAmplitude;
		TransformSystem::Get(bm.transformId).position = pos;
		TransformSystem::UpdateMatrices(bm.transformId);
	}

	scene.mainCamera->Update(input, deltaTime);
	renderPipeline.Render(scene, input, deltaTime, runningTime);

	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
