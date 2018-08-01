#include "TestApp.h"

#include <imgui.h>

#include "GuiSystem.h"
#include "ShaderSystem.h"
#include "TransformSystem.h"
#include "TextureSystem.h"
#include "ModelSystem.h"

#include "FpsCamera.h"
#include "GBuffer.h"

#include "GeometryPass.h"

#include "shader_locations.h"
#include "camera_uniforms.h"

///////////////////////////////////////////////////////////////////////////////
// Data

std::vector<Model> models;

GBuffer gBuffer;
FpsCamera camera;

Model testQuad;

// Passes
GeometryPass geometryPass;

///////////////////////////////////////////////////////////////////////////////
// Application lifetime

App::Settings TestApp::Setup()
{
	Settings settings{};
	settings.window.size = { 1280, 800 };
	settings.window.resizeable = true;
	settings.context.msaaSamples = 4;
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
		}
	});

	ModelSystem::LoadModel("assets/quad/quad.obj");
	ModelSystem::LoadModel("assets/sponza/sponza.obj");

	camera.LookAt({ -20, 3, 0 }, { 0, 10, 0 });
}

void TestApp::Resize(int width, int height)
{
	glViewport(0, 0, width, height);
	camera.Resize(width, height);

	gBuffer.RecreateGpuResources(width, height);
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

	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
