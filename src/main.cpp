#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <string>
#include <sstream>
#include <fstream>
#include <cassert>
#include <stdlib.h>

#include "Logging.h"
#include "ShaderSystem.h"

#include "mesh_attributes.h"

// _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

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
    // ...
}

//
// Utility functions
//

GLuint loadImage(const char *filePath)
{
    stbi_set_flip_vertically_on_load(true);

    int width, height, numComponents;
    unsigned char *pixels = stbi_load(filePath, &width, &height, &numComponents, 0);

    if (pixels == nullptr) {
        LogError("Image loading error ('%s'): %s. Aborting.\n", filePath, stbi_failure_reason());
    }

    GLenum format, internalFormat;
    switch (numComponents)
    {
        case 1:
            format = GL_RED;
            internalFormat = GL_R8;
            break;
        case 2:
            format = GL_RG;
            internalFormat = GL_RG8;
            break;
        case 3:
            format = GL_RGB;
            internalFormat = GL_RGB8;
            break;
        case 4:
            format = GL_RGBA;
            internalFormat = GL_RGBA8;
            break;
        default:
			LogError("Image loading error: unknown number of components (%d). Aborting.", numComponents);
			break;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);

    // Set some defaults. Min-filter is required!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(pixels);
    return texture;
}

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

    // Setup test shader program
	ShaderSystem shaderSystem{ "shaders/" };
	GLuint* program = shaderSystem.AddProgram("default");
    
    // Setup test image
    GLuint testScreen = loadImage("assets/test_pattern.png");

    // Setup test geometry
    GLuint quad;
    {
        glGenVertexArrays(1, &quad);
        glBindVertexArray(quad);

        // Positions
        float positions[] = {
            -1.0f, -1.0f,
            -1.0f, +1.0f,
            +1.0f, +1.0f,

            -1.0f, -1.0f,
            +1.0f, +1.0f,
            +1.0f, -1.0f
        };
        GLuint positionsBuffer;
        glGenBuffers(1, &positionsBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, positionsBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(MESH_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // Texture coordinates
        float UVs[] = {
            0, 0,
            0, 1,
            1, 1,

            0, 0,
            1, 1,
            1, 0
        };
        GLuint uvBuffer;
        glGenBuffers(1, &uvBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(UVs), UVs, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(MESH_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindVertexArray(0);

        glDeleteBuffers(1, &positionsBuffer);
        glDeleteBuffers(1, &uvBuffer);
    }


    // Render loop
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(window))
	{
		shaderSystem.Update();
		glfwPollEvents();

        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        {
			glUseProgram(*program);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, testScreen);
            glUniform1i(glGetUniformLocation(*program, "u_texture"), 0);

            glBindVertexArray(quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
}
