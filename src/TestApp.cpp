#include "TestApp.h"

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

#include "GeometryPass.h"
#include "ShadowPass.h"
#include "LightPass.h"

using namespace glm;
#include "shader_types.h"
#include "shader_locations.h"
#include "camera_uniforms.h"

///////////////////////////////////////////////////////////////////////////////
// Data

FpsCamera camera;
std::vector<Model> models;

GBuffer gBuffer;
ShadowMap shadowMap;
LightBuffer lightBuffer;

Model testQuad;

// Lights
DirectionalLight sunLight;

// Passes
GeometryPass geometryPass;
ShadowPass shadowPass;
LightPass lightPass;

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
		models.emplace_back(model);
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

	sunLight.worldDirection = glm::vec4(-0.2, -1.0, -0.2, 0);
	sunLight.color = glm::vec4(1.0, 0.9, 0.9, 0.1);

	camera.LookAt({ -20, 3, 0 }, { 0, 10, 0 });
}

void TestApp::Resize(int width, int height)
{
	camera.Resize(width, height);

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
		ImGui::SetWindowPos(ImVec2(10, 10));
		ImGui::SetWindowSize(ImVec2(350, 500));
	}

	static bool showMetrics = false;
	ImGui::Checkbox("Show metrics", &showMetrics);
	if (showMetrics)
	{
		ImGui::ShowMetricsWindow();
	}

	camera.Update(input, deltaTime);

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

	// All geometry is currently opaque! Maybe later add some material flag to indicate opaqueness?
	auto opaqueGeometry = std::vector<Model>(models);

	geometryPass.performDepthPrepass = true;
	geometryPass.Draw(gBuffer, opaqueGeometry, camera);

	if (ImGui::CollapsingHeader("G-Buffer"))
	{
		ImGui::Text("Albedo:");
		GuiSystem::Texture(gBuffer.albedoTexture);
		ImGui::Text("Normal:");
		GuiSystem::Texture(gBuffer.normalTexture);
		ImGui::Text("Depth:");
		GuiSystem::Texture(gBuffer.depthTexture);
	}

	shadowPass.Draw(shadowMap, models, sunLight);

	if (ImGui::CollapsingHeader("Shadows"))
	{
		GuiSystem::Texture(shadowMap.texture, 1.0f);
	}

	lightPass.Draw(lightBuffer, gBuffer, shadowMap, /*lights,*/ camera, sunLight);

	if (ImGui::CollapsingHeader("Light buffer"))
	{
		GuiSystem::Texture(lightBuffer.lightTexture);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
