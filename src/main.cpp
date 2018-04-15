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

#include "mesh_attributes.h"

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

void glfw_mouse_button_callback(GLFWwindow *window, int button, int state, int mods)
{
	// ...
}

void glfw_key_callback(GLFWwindow *window, int key, int scancode, int state, int mods)
{
	if (key == GLFW_KEY_ESCAPE)
	{
		glfwSetWindowShouldClose(window, true);
	}

	if (state == GLFW_REPEAT || state == GLFW_PRESS)
	{
		const float speed = 0.01f;
		float dx = 0.0f;
		float dy = 0.0f;

		if (key == GLFW_KEY_RIGHT) dx += speed;
		if (key == GLFW_KEY_LEFT)  dx -= speed;
		if (key == GLFW_KEY_UP)    dy += speed;
		if (key == GLFW_KEY_DOWN)  dy -= speed;

		for (auto model : models)
		{
			auto& transform = transforms.Get(model.transformID);
			transform.position.x += dx;
			transform.position.y += dy;
		}
	}
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
	GLFWwindow *window = glfwCreateWindow(width, height, "Prospect Renderer", nullptr, nullptr);
	if (!window) exit(EXIT_FAILURE);

	// Setup basic event listeners
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
	glfwSetKeyCallback(window, glfw_key_callback);

	// Setup OpenGL context
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glDebugMessageCallback(gl_debug_message_callback, nullptr);

	// Enable important and basic features
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	ShaderSystem shaderSystem{ "shaders/" };
	GLuint* program = shaderSystem.AddProgram("default");

	TextureSystem textureSystem;
	GLuint texture = textureSystem.LoadLdrImage("assets/bricks_col.jpg");

	ModelSystem modelSystem{ [&](Model model, const std::string& filename, const std::string& modelname)
	{
		models.emplace_back(model);
	}};
	
	modelSystem.LoadModel("assets/quad/quad.obj");
	modelSystem.LoadModel("assets/teapot/teapot.obj");

	// Render loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window))
	{
		modelSystem.Update();
		shaderSystem.Update();
		textureSystem.Update();

		glfwPollEvents();

		glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		{
			glUseProgram(*program);

			GLint location = glGetUniformLocation(*program, "u_texture");
			GLuint unit = 0;

			glBindTextureUnit(unit, texture);
			glUniform1i(location, unit);
			
			for (auto& model : models)
			{
				transforms.UpdateMatrices(model.transformID);
				auto& transform = transforms.Get(model.transformID);

				GLint worldFromLocalLocation = glGetUniformLocation(*program, "u_world_from_local");
				glUniformMatrix4fv(worldFromLocalLocation, 1, GL_FALSE, glm::value_ptr(transform.matrix));

				model.Draw();
			}
		}

		glfwSwapBuffers(window);
	}

	glfwTerminate();
}
