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
#include "MaterialSystem.h"
#include "TransformSystem.h"

#include "Input.h"
#include "TestApp.h"

// _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

//
// Globals
//

std::unique_ptr<App> app;

//
// Callbacks
//

void gl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	Log("GL debug message: %s\n", message);
}

void glad_post_callback(const char *name, void *funcptr, int argCount, ...)
{
	// Use the "raw" glad_glGetError to avoid an recursive loop
	GLenum errorCode = glad_glGetError();

	if (errorCode != GL_NO_ERROR)
	{
		const char *errorName;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:
			errorName = "GL_INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			errorName = "GL_INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			errorName = "GL_INVALID_OPERATION";
			break;
		case GL_OUT_OF_MEMORY:
			errorName = "GL_OUT_OF_MEMORY";
			break;
		default:
			errorName = "UnknownError";
			break;
		}

		LogError("%s error (0x%03x) in '%s'\n", errorName, errorCode, name);
	}
}

void glfw_error_callback(int code, const char *message)
{
	LogError("GLFW error %d: %s\n", code, message);
}

void glfw_framebuffer_resize_callback(GLFWwindow *window, int width, int height)
{
	app->Resize(width, height);
}

//
// Global handlers
//

void handle_global_key_commands(GLFWwindow* window, const Input& input)
{
	// Mouse lock & unlock
	if (input.WasKeyPressed(GLFW_KEY_F1))
	{
		int currentMode = glfwGetInputMode(window, GLFW_CURSOR);
		int newMode = (currentMode == GLFW_CURSOR_NORMAL) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
		glfwSetInputMode(window, GLFW_CURSOR, newMode);
	}

	// Essentially reset everything back to startup state
	if (input.WasKeyPressed(GLFW_KEY_ESCAPE))
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

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

	// Create app (should require no GL context!)
	app = std::make_unique<TestApp>();
	App::Settings settings = app->Setup();

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

	// Setup OpenGL error handlers
	glad_set_post_callback(glad_post_callback);
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

	// Initialize global systems (that need initialization)
	TextureSystem::Init();
	MaterialSystem::Init();
	ModelSystem::Init();

	app->Init();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_resize_callback);
	app->Resize(width, height);

	glfwSwapInterval(settings.window.vsync ? 1 : 0);

	// Main loop

	glfwSetTime(0.0);
	double lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		input.PreEventPoll();
		glfwPollEvents();

		TextureSystem::Update();
		ModelSystem::Update();
		ShaderSystem::Update();

		handle_global_key_commands(window, input);

		double currentTime = glfwGetTime();
		double elapsedTime = currentTime - lastTime;
		lastTime = currentTime;

		float deltaTime = float(elapsedTime);
		app->Draw(input, deltaTime);

		glfwSwapBuffers(window);
	}

	// Destroy global systems (that need to be destroyed)
	TextureSystem::Destroy();
	MaterialSystem::Destroy();
	ModelSystem::Destroy();

	glfwTerminate();
}
