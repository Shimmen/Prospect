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

	//
	// Utilities
	//

	void Texture(GLuint texture, float aspectRatio = 16.0f / 9.0f);

	void SnapSliderFloat(const char *label, float *value, const float *steps, int stepCount, const char *displayFormat = "%.3f");

}
