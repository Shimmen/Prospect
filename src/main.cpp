#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <sstream>
#include <fstream>
#include <cassert>
#include <stdlib.h>

#include "Logging.h"
#include "ModelSystem.h"
#include "ShaderSystem.h"
#include "TextureSystem.h"
#include "TransformSystem.h"

#include "Input.h"

#include "FpsCamera.h"

#include "mesh_attributes.h"
#include "uniform_locations.h"

// _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

//
// Globals
//

std::vector<Model> models;
TransformSystem transforms;

//
// Callbacks
//

void gl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	Log("GL debug message: %s\n", message);
}

void glfw_error_callback(int code, const char *message)
{
	LogError("GLFW error %d: %s\n", code, message);
}

// TODO: Redirect glad debug callbacks to something clever here!

//
//
//

int main()
{
	const int width = 1280;
	const int height = 800;

	// Setup window & context
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) exit(EXIT_FAILURE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // required for macOS
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *vidmode = glfwGetVideoMode(monitor);
	GLFWwindow *window = glfwCreateWindow(vidmode->width, vidmode->height, "Prospect Renderer", monitor, nullptr);
	if (!window) exit(EXIT_FAILURE);

	// Setup input
	Input input;
	glfwSetWindowUserPointer(window, &input);
	glfwSetKeyCallback(window, Input::KeyEventCallback);
	glfwSetCharModsCallback(window, Input::CharEventCallback);
	glfwSetMouseButtonCallback(window, Input::MouseButtonEventCallback);
	glfwSetCursorPosCallback(window, Input::MouseMovementEventCallback);
	glfwSetScrollCallback(window, Input::MouseScrollEventCallback);

	// Setup OpenGL context
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glDebugMessageCallback(gl_debug_message_callback, nullptr);

	// Enable important and basic features (plus some initial state)
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_DEPTH_TEST);

	ShaderSystem shaderSystem{ "shaders/" };
	GLuint* program = shaderSystem.AddProgram("default");

	TextureSystem textureSystem;
	GLuint texture = textureSystem.LoadLdrImage("assets/bricks_col.jpg");

	ModelSystem modelSystem{ [&](Model model, const std::string& filename, const std::string& modelname)
	{
		models.emplace_back(model);
	}};
	
	modelSystem.LoadModel("assets/quad/quad.obj");
	modelSystem.LoadModel("assets/sponza/sponza.obj");

	FpsCamera camera;

	// Render loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window))
	{
		modelSystem.Update();
		shaderSystem.Update();
		textureSystem.Update();

		input.PreEventPoll();
		glfwPollEvents();

		float dt = 1.0f / 60.0f;
		camera.Update(input, dt);

		glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		{
			glUseProgram(*program);

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

		glfwSwapBuffers(window);
	}

	glfwTerminate();
}
