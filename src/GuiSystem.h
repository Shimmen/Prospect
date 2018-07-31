#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>

#include "Input.h"

namespace GuiSystem
{
	void Init(GLFWwindow *window);
	void Destroy();

	void NewFrame(const Input& input, float deltaTime);
	void RenderDrawData(ImDrawData *data);

	bool IsUsingMouse();
	bool IsUsingKeyboard();

	// The Input class doesn't capture character/text input, so this is managed here instead
	void CharacterInputCallback(GLFWwindow *window, unsigned int codepoint);

}
