#include "GuiSystem.h"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ShaderSystem.h"

#include "shader_locations.h"

//
// Data
//

static GLFWwindow *window;

static GLuint *shaderProgram;

static GLuint vertexArray;
static GLuint vertexBuffer;
static GLuint indexBuffer;
static GLuint fontTexture;

static GLFWcursor *cursors[ImGuiMouseCursor_COUNT] = { 0 };


//
// Public API
//

void
GuiSystem::Init(GLFWwindow *targetWindow)
{
	window = targetWindow;
	ImGui::CreateContext();

	shaderProgram = ShaderSystem::AddProgram("gui/gui");

	// Setup GUI vertex array and buffers

	glCreateVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glCreateBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(ImDrawVert), (GLvoid *)offsetof(ImDrawVert, pos));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(ImDrawVert), (GLvoid *)offsetof(ImDrawVert, uv));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(ImDrawVert), (GLvoid *)offsetof(ImDrawVert, col));

	glCreateBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	auto& io = ImGui::GetIO();

	// Setup the default font texture

	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	glCreateTextures(GL_TEXTURE_2D, 1, &fontTexture);
	glTextureStorage2D(fontTexture, 1, GL_RGBA8, width, height);
	glTextureSubImage2D(fontTexture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glTextureParameteri(fontTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(fontTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	io.Fonts->TexID = (ImTextureID)static_cast<uint64_t>(fontTexture);

	// Setup the keyboard mapping.
	// (ImGui will use those indices to peek into the io.KeyDown[] array)

	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

	// Setup copy/paste operations

	io.SetClipboardTextFn = [](void *userData, const char *text)
	{
		glfwSetClipboardString(window, text);
	};

	io.GetClipboardTextFn = [](void *userData)
	{
		return glfwGetClipboardString(window);
	};

	// Setup mouse cursors

	cursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	cursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
	cursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	cursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
	cursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
	cursors[ImGuiMouseCursor_ResizeNESW] = nullptr; // (not standard glfw cursor)
	cursors[ImGuiMouseCursor_ResizeNWSE] = nullptr; // (not standard glfw cursor)
}

void
GuiSystem::Destroy()
{
	glDeleteVertexArrays(1, &vertexArray);
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteBuffers(1, &indexBuffer);
	glDeleteTextures(1, &fontTexture);

	for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i)
	{
		if (cursors[i])
		{
			glfwDestroyCursor(cursors[i]);
			cursors[i] = nullptr;
		}
	}
}

void
GuiSystem::NewFrame(const Input& input, float deltaTime)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;

	// Setup display size

	int w, h;
	int display_w, display_h;
	glfwGetWindowSize(window, &w, &h);
	glfwGetFramebufferSize(window, &display_w, &display_h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

	// Update mouse position

	const ImVec2 mouse_pos_backup = io.MousePos;
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	if (glfwGetWindowAttrib(window, GLFW_FOCUSED))
	{
		if (io.WantSetMousePos)
		{
			glfwSetCursorPos(window, (double)mouse_pos_backup.x, (double)mouse_pos_backup.y);
		}
		else
		{
			double mouse_x, mouse_y;
			glfwGetCursorPos(window, &mouse_x, &mouse_y);
			io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
		}
	}

	// Update mouse scroll and buttons

	io.MouseWheel += input.GetScrollDelta();

	int numMouseButtons = std::min(Input::MOUSE_BUTTON_COUNT, IM_ARRAYSIZE(io.MouseDown));
	for (int i = 0; i < numMouseButtons; ++i)
	{
		io.MouseDown[i] = input.IsButtonDown(i);
	}

	// Update keyboard input

	io.KeyAlt   = input.IsKeyDown(GLFW_KEY_LEFT_ALT) || input.IsKeyDown(GLFW_KEY_RIGHT_ALT);
	io.KeyCtrl  = input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || input.IsKeyDown(GLFW_KEY_RIGHT_CONTROL);
	io.KeyShift = input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || input.IsKeyDown(GLFW_KEY_RIGHT_SHIFT);
	io.KeySuper = input.IsKeyDown(GLFW_KEY_LEFT_SUPER) || input.IsKeyDown(GLFW_KEY_RIGHT_SUPER);

	int numKeys = std::min(Input::KEYBOARD_KEY_COUNT, IM_ARRAYSIZE(io.KeysDown));
	for (int i = 0; i < numKeys; ++i)
	{
		io.KeysDown[i] = input.IsKeyDown(i);
	}
	
	// Update cursors

	if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
	{
		ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
		if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
		{
			// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}
		else
		{
			// Show OS mouse cursor
			glfwSetCursor(window, cursors[imgui_cursor] ? cursors[imgui_cursor] : cursors[ImGuiMouseCursor_Arrow]);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	ImGui::NewFrame();
}

void
GuiSystem::RenderDrawData(ImDrawData *data)
{
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	ImGuiIO& io = ImGui::GetIO();
	int fbWidth = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fbHeight = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	if (fbWidth <= 0 || fbHeight <= 0)
	{
		return;
	}

	data->ScaleClipRects(io.DisplayFramebufferScale);

	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Setup viewport, orthographic projection matrix
	glViewport(0, 0, (GLsizei)fbWidth, (GLsizei)fbHeight);
	glm::mat4 projection = glm::mat4{
		2.0f / io.DisplaySize.x, 0.0f,                   0.0f, 0.0f,
		0.0f,                  2.0f / -io.DisplaySize.y, 0.0f, 0.0f,
		0.0f,                  0.0f,                  -1.0f, 0.0f,
		-1.0f,                 1.0f,                   0.0f, 1.0f
	};

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindVertexArray(vertexArray);
	glUseProgram(*shaderProgram);

	GLint textureUnit = 0;
	glUniform1i(PredefinedUniformLocation(u_gui_texture), textureUnit);
	glUniformMatrix4fv(PredefinedUniformLocation(u_gui_projection), 1, GL_FALSE, glm::value_ptr(projection));

	// Perform drawing

	ImVec2 pos = ImVec2(0, 0); //data->DisplayPos; ?? what is this property?
	for (int n = 0; n < data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = data->CmdLists[n];
		const ImDrawIdx* idx_buffer_offset = 0;

		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				// User callback (registered via ImDrawList::AddCallback)
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				ImVec4 clip_rect = ImVec4(pcmd->ClipRect.x - pos.x, pcmd->ClipRect.y - pos.y, pcmd->ClipRect.z - pos.x, pcmd->ClipRect.w - pos.y);
				if (clip_rect.x < fbWidth && clip_rect.y < fbHeight && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
				{
					// Apply scissor/clipping rectangle
					glScissor((int)clip_rect.x, (int)(fbHeight - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

					// Bind texture, Draw
					glBindTextureUnit(textureUnit, (GLuint)(intptr_t)pcmd->TextureId);
					glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
				}
			}
			idx_buffer_offset += pcmd->ElemCount;
		}
	}

	// Restore to a "sane default" ... yeah, not great.
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glBindVertexArray(0);
}

bool
GuiSystem::IsUsingMouse()
{
	ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureMouse;
}

bool
GuiSystem::IsUsingKeyboard()
{
	ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureKeyboard;
}

void
GuiSystem::CharacterInputCallback(GLFWwindow *window, unsigned int codepoint)
{
	if (codepoint > 0 && codepoint < 0x10000)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.AddInputCharacter((unsigned short)codepoint);
	}
}

void
GuiSystem::Texture(GLuint texture, float aspectRatio)
{
	if (texture == 0) return;

	ImVec2 windowSize = ImGui::GetWindowSize();
	float width = windowSize.x;
	float height = width / aspectRatio;

	const ImVec2 uv0{ 0, 1 };
	const ImVec2 uv1{ 1, 0 };

	auto imageId = (ImTextureID)static_cast<uint64_t>(texture);
	ImGui::Image(imageId, ImVec2(width, height), uv0, uv1);
}

void
GuiSystem::SnapSliderFloat(const char *label, float *value, const float *steps, int stepCount, const char *displayFormat)
{
	ImGui::SliderFloat(label, value, steps[0], steps[stepCount - 1], displayFormat);

	// Snap to closest index
	int index = 1;
	for (; index < stepCount && *value >= steps[index]; index += 1) { /* no-op */ }

	float distUp = abs(steps[index] - *value);
	float distDown = abs(steps[index - 1] - *value);
	if (distDown < distUp) index -= 1;

	*value = steps[index];
}
