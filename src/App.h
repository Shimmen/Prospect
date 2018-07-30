#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Input.h"
#include "Logging.h"

#include "ModelSystem.h"
#include "ShaderSystem.h"
#include "TextureSystem.h"

class App
{
public:

	struct Settings
	{
		Settings() = default;

		struct
		{
			std::string title{ "Prospect Renderer" };
			union
			{
				struct { int width; int height; };
				bool fullscreen = false;
			} size;
			bool resizeable = false;
			bool vsync = true;
		} window;

		struct
		{
			int msaaSamples = 1;
		} context;
	};

public:

	App() = default;
	virtual ~App() = default;

	///////////////////////////////////////////////////////////////////////////
	// Application lifetime

	// Called before any window or context is created, so must not call any GL code
	virtual Settings Setup() = 0;

	// Called on app creation. Window etc. does exist by this point
	virtual void Init() = 0;

	// Called on default framebuffer resize, and right after Init
	virtual void Resize(int width, int height) = 0;

	///////////////////////////////////////////////////////////////////////////
	// Drawing / main loop

	// Called every new frame
	virtual void Draw(const Input& input, float deltaTime, float runningTime) = 0;

};
