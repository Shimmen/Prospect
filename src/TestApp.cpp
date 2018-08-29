#include "TestApp.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include <imgui.h>

#include "GuiSystem.h"
#include "ShaderSystem.h"
#include "TransformSystem.h"
#include "TextureSystem.h"
#include "ModelSystem.h"

#include "FpsCamera.h"
#include "GBuffer.h"
#include "LightBuffer.h"
#include "ShadowMap.h"

#include "Scene.h"

#include "GeometryPass.h"
#include "ShadowPass.h"
#include "LightPass.h"
#include "FinalPass.h"

using namespace glm;
#include "shader_types.h"
#include "shader_locations.h"
#include "camera_uniforms.h"

///////////////////////////////////////////////////////////////////////////////
// Data

Scene scene{};
Model testQuad;

GBuffer gBuffer;
ShadowMap shadowMap;
LightBuffer lightBuffer;

GeometryPass geometryPass;
ShadowPass shadowPass;
LightPass lightPass;
FinalPass finalPass;

///////////////////////////////////////////////////////////////////////////////
// Application lifetime

App::Settings TestApp::Setup()
{
	Settings settings{};
	settings.window.size = { 1280, 800 };
	settings.window.resizeable = true;
	settings.context.msaaSamples = 1;
	return settings;
}

void TestApp::Init()
{
	ModelSystem::SetModelLoadCallback([&](Model model, const std::string& filename, const std::string& modelname)
	{
		scene.models.emplace_back(model);
		//6Log("Model %s from file %s loaded\n", modelname.c_str(), filename.c_str());

		// TODO: Create proper API for these types of operations
		if (filename == "assets/quad/quad.obj")
		{
			testQuad = model;
			testQuad.material->cullBackfaces = false;
		}
	});

	ModelSystem::LoadModel("assets/quad/quad.obj");
	ModelSystem::LoadModel("assets/sponza/sponza.obj");

	shadowMap.RecreateGpuResources(8192);

	DirectionalLight sunLight;
	sunLight.worldDirection = glm::vec4(-0.2, -1.0, -0.2, 0);
	sunLight.color = glm::vec4(1.0, 0.9, 0.9, 0.1);
	scene.directionalLights.push_back(sunLight);

	scene.mainCamera.LookAt({ -20, 3, 0 }, { 0, 10, 0 });
}

void TestApp::Resize(int width, int height)
{
	scene.mainCamera.Resize(width, height);

	gBuffer.RecreateGpuResources(width, height);
	lightBuffer.RecreateGpuResources(width, height);
}

///////////////////////////////////////////////////////////////////////////////
// Drawing / main loop

void TestApp::Draw(const Input& input, float deltaTime, float runningTime)
{
	ImGui::Begin("Prospect");
	if (input.WasKeyPressed(GLFW_KEY_HOME))
	{
		ImGui::SetWindowPos(ImVec2(0, 1));
		ImGui::SetWindowSize(ImVec2(350.0f, lightBuffer.height - 25.0f));
	}

	scene.mainCamera.Update(input, deltaTime);

	for (auto& dirLight : scene.directionalLights)
	{
		dirLight.worldDirection = glm::rotateY(dirLight.worldDirection, deltaTime);
	}

	// TODO: Make some better API for these types of things
	if (testQuad.vao)
	{
		auto& quadTransform = TransformSystem::Get(testQuad.transformID);
		{
			quadTransform.position.x = 4.0f * std::cos(runningTime);
			quadTransform.position.z = 3.0f * std::sin(runningTime);
			quadTransform.position.y = 5.0f + 0.25f * std::sin(runningTime * 10.0f);
			quadTransform.orientation = glm::rotate(quadTransform.orientation, deltaTime, { 0, 1, 0 });
		}
		TransformSystem::UpdateMatrices(testQuad.transformID);
	}

	//

	geometryPass.performDepthPrepass = true;
	geometryPass.Draw(gBuffer, scene);

	shadowPass.Draw(shadowMap, scene);
	lightPass.Draw(lightBuffer, gBuffer, shadowMap, scene);

	finalPass.Draw(lightBuffer);

	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
