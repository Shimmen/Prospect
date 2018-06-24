#include "TestApp.h"

#include "ShaderSystem.h"
#include "TransformSystem.h"
#include "MaterialSystem.h"
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

GLuint* program;
GLuint texture;

FpsCamera camera;

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
	texture = TextureSystem::LoadLdrImage("assets/bricks_col.jpg");

	ModelSystem::SetModelLoadCallback([&](Model model, const std::string& filename, const std::string& modelname)
	{
		models.emplace_back(model);
	});

	ModelSystem::LoadModel("assets/quad/quad.obj");
	ModelSystem::LoadModel("assets/sponza/sponza.obj");
}

void TestApp::Resize(int width, int height)
{
	glViewport(0, 0, width, height);
	camera.Resize(width, height);

	gBuffer.RecreateGpuResources(width, height);
}

///////////////////////////////////////////////////////////////////////////////
// Drawing / main loop

void TestApp::Draw(const Input& input, float deltaTime)
{
	camera.Update(input, deltaTime);

	//

	// All geometry is currently opaque! Maybe later add some material flag to indicate opaqueness?
	auto opaqueGeometry = std::vector<Model>(models);

	// TODO: Perform depth prepass!

	geometryPass.Draw(gBuffer, opaqueGeometry, camera);

}

///////////////////////////////////////////////////////////////////////////////
