#include "TestApp.h"

#include "ShaderSystem.h"
#include "TransformSystem.h"
#include "TextureSystem.h"
#include "ModelSystem.h"

#include "FpsCamera.h"

#include "uniform_locations.h"

///////////////////////////////////////////////////////////////////////////////
// Data

std::vector<Model> models;
TransformSystem transforms;

GLuint* program;
GLuint texture;

FpsCamera camera;

///////////////////////////////////////////////////////////////////////////////
// Application lifetime

App::Settings TestApp::Setup()
{
	Settings settings{};
	settings.window.size = { 1280, 800 };
	settings.window.resizeable = false;
	settings.context.msaaSamples = 4;
	return settings;
}

void TestApp::Init()
{
	program = shaderSystem.AddProgram("default");
	texture = textureSystem.LoadLdrImage("assets/bricks_col.jpg");

	modelSystem.LoadModel("assets/quad/quad.obj");
	modelSystem.LoadModel("assets/sponza/sponza.obj");
}

void TestApp::Resize(int width, int height)
{
	glViewport(0, 0, width, height);
	camera.Resize(width, height);
}

///////////////////////////////////////////////////////////////////////////////
// Drawing / main loop

void TestApp::Draw(const Input& input, float deltaTime)
{
	modelSystem.Update();
	shaderSystem.Update();
	textureSystem.Update();

	camera.Update(input, deltaTime);

	//

	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(*program);
	glEnable(GL_DEPTH_TEST);

	// Camera uniforms
	glUniformMatrix4fv(PredefinedUniformLocation(u_view_from_world), 1, GL_FALSE, glm::value_ptr(camera.GetViewMatrix()));
	glUniformMatrix4fv(PredefinedUniformLocation(u_projection_from_view), 1, GL_FALSE, glm::value_ptr(camera.GetProjectionMatrix()));

	for (auto& model : models)
	{
		// Object matrices
		transforms.UpdateMatrices(model.transformID);
		auto& transform = transforms.Get(model.transformID);
		glUniformMatrix4fv(PredefinedUniformLocation(u_world_from_local), 1, GL_FALSE, glm::value_ptr(transform.matrix));

		// Texture uniforms and bindings

		//
		//    For the future:
		//
		// int unit = 0;
		// for each texture in the current material
		//   glBindTextureUnit(unit, texture);
		//   unit += 1;
		//
		// HOWEVER, maybe we want to set uniform OR texture units in advance.
		// I.e. set one of them and let the other vary.
		//

		GLuint unit = 0;
		glBindTextureUnit(unit, texture);
		int a = PredefinedUniformLocation(u_diffuse);
		glUniform1i(PredefinedUniformLocation(u_diffuse), unit);

		model.Draw();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Events

void TestApp::OnModelLoad(Model model, const std::string& filename, const std::string& modelname)
{
	models.emplace_back(model);
}

///////////////////////////////////////////////////////////////////////////////
