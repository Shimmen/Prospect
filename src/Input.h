#pragma once

#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>

class Input
{
public:

	Input() = default;
	~Input() = default;

	Input(Input& other) = delete;
	Input& operator=(Input& other) = delete;

	void PreEventPoll();

	//
	// Keyboard related
	//

	bool IsKeyDown(int key) const;
	bool WasKeyPressed(int key) const;
	bool WasKeyReleased(int key) const;

	static void KeyEventCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void CharEventCallback(GLFWwindow *window, unsigned int codepoint, int mods);

	//
	// Mouse related
	//

	bool IsButtonDown(int button) const;
	bool WasButtonPressed(int button) const;
	bool WasButtonReleased(int button) const;

	glm::vec2 GetMousePosition(GLFWwindow *window) const;
	glm::vec2 GetMouseDelta() const;
	float GetScrollDelta() const;

	static void MouseButtonEventCallback(GLFWwindow *window, int button, int action, int mods);
	static void MouseMovementEventCallback(GLFWwindow *window, double xPos, double yPos);
	static void MouseScrollEventCallback(GLFWwindow *window, double xOffset, double yOffset);

private:

	//
	// Keyboard related
	//

	static const int KEYBOARD_KEY_COUNT{ GLFW_KEY_LAST };

	bool isKeyDown[KEYBOARD_KEY_COUNT] = { 0 };
	bool wasKeyPressed[KEYBOARD_KEY_COUNT] = { 0 };
	bool wasKeyReleased[KEYBOARD_KEY_COUNT] = { 0 };

	//
	// Mouse related
	//

	static const int MOUSE_BUTTON_COUNT{ GLFW_MOUSE_BUTTON_LAST };

	bool isButtonDown[MOUSE_BUTTON_COUNT] = { 0 };
	bool wasButtonPressed[MOUSE_BUTTON_COUNT] = { 0 };
	bool wasButtonReleased[MOUSE_BUTTON_COUNT] = { 0 };

	// Should only be used for getting the mouse delta etc. and NOT for querying current position, since this accumulates errors and might "lag" behind.
	double currentXPosition{ 0 };
	double currentYPosition{ 0 };
	double lastXPosition{ -1 };
	double lastYPosition{ -1 };

	double currentScollOffset{ 0 };
	double lastScrollOffset{ 0 };

};
