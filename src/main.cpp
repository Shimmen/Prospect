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
#include "TestApp.h"

// _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

//
// Globals
//

// ...

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
	// Setup basic GLFW and context settings
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		LogError("Fatal error: could not initialize GLFW");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // required for macOS

	// Create app (should require no GL context!)
	TestApp app{};
	App::Settings settings = app.Setup();

	glfwWindowHint(GLFW_RESIZABLE, settings.window.resizeable);
	glfwWindowHint(GLFW_SAMPLES, settings.context.msaaSamples);

	// Create window
	GLFWwindow *window = nullptr;
	if (settings.window.size.fullscreen)
	{
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *vidmode = glfwGetVideoMode(monitor);
		window = glfwCreateWindow(vidmode->width, vidmode->height, settings.window.title.c_str(), monitor, nullptr);
	}
	else
	{
		int width = settings.window.size.width;
		int height = settings.window.size.height;
		window = glfwCreateWindow(width, height, settings.window.title.c_str(), nullptr, nullptr);
	}
	if (!window)
	{
		LogError("Fatal error: could not created the requested window");
	}

	// Setup OpenGL context
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glDebugMessageCallback(gl_debug_message_callback, nullptr);

	// Setup input
	Input input;
	glfwSetWindowUserPointer(window, &input);
	glfwSetKeyCallback(window, Input::KeyEventCallback);
	glfwSetCharModsCallback(window, Input::CharEventCallback);
	glfwSetMouseButtonCallback(window, Input::MouseButtonEventCallback);
	glfwSetCursorPosCallback(window, Input::MouseMovementEventCallback);
	glfwSetScrollCallback(window, Input::MouseScrollEventCallback);

	// Enable important and basic features (plus some initial state)
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	app.Init();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	app.Resize(width, height);

	// Render loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window))
	{
		input.PreEventPoll();
		glfwPollEvents();

		float dt = 1.0f / 60.0f;
		app.Draw(input, dt);
		

		glfwSwapBuffers(window);
	}

	glfwTerminate();
}
