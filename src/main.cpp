#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <string>
#include <sstream>
#include <fstream>
#include <cassert>
#include <stdlib.h>

#include "Logging.h"
#include "ModelSystem.h"
#include "ShaderSystem.h"
#include "TextureSystem.h"

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

	std::vector<Model> models;
	ModelSystem modelSystem{ [&](Model model, const std::string& filename, const std::string& modelname)
	{
		models.emplace_back(model);
	}};
	
	modelSystem.LoadModel("assets/quad/quad.obj");
	modelSystem.LoadModel("assets/teapot/teapot.obj");

    // Setup test geometry
	/*
	Model quad;
    {
		GLuint indexBuffer;
		GLsizei indexCount;
		GLenum  indexType;
		{
			glCreateBuffers(1, &indexBuffer);

			unsigned int indices[] = {
				0, 1, 2, 3, 4, 5
			};

			indexCount = static_cast<GLsizei>(sizeof(indices));
			indexType = GL_UNSIGNED_INT;

			size_t size = sizeof(uint32_t) * indexCount;

			GLbitfield flags = GL_DYNAMIC_STORAGE_BIT;
			glNamedBufferStorage(indexBuffer, size, indices, flags);
		}

		GLuint positionsBuffer;
		{
			glCreateBuffers(1, &positionsBuffer);

			float positions[] = {
				-1.0f, -1.0f,
				-1.0f, +1.0f,
				+1.0f, +1.0f,

				-1.0f, -1.0f,
				+1.0f, +1.0f,
				+1.0f, -1.0f
			};

			size_t size = sizeof(positions);
			GLbitfield flags = GL_DYNAMIC_STORAGE_BIT;
			glNamedBufferStorage(positionsBuffer, size, positions, flags);
		}

		GLuint texCoordBuffer;
		{
			glCreateBuffers(1, &texCoordBuffer);

			float UVs[] = {
				0, 0,
				0, 1,
				1, 1,

				0, 0,
				1, 1,
				1, 0
			};

			size_t size = sizeof(UVs);
			GLbitfield flags = GL_DYNAMIC_STORAGE_BIT;
			glNamedBufferStorage(texCoordBuffer, size, UVs, flags);
		}

		GLuint vao;
		glCreateVertexArrays(1, &vao);

		// Specify the element buffer for this vertex array
		glVertexArrayElementBuffer(vao, indexBuffer);

		// Bind the vertex array to a specific binding index and specify it stride, etc.
		GLuint positionsBindingIndex = 0;
		GLuint texCoordsBindingIndex = 1;
		glVertexArrayVertexBuffer(vao, positionsBindingIndex, positionsBuffer, 0, 2 * sizeof(float));
		glVertexArrayVertexBuffer(vao, texCoordsBindingIndex, texCoordBuffer , 0, 2 * sizeof(float));

		// Enable the attribute, specify its format, and connect the vertex array (at its
		// binding index) to to this specific attribute for this vertex array
		glEnableVertexArrayAttrib(vao, MESH_ATTRIB_POSITION);
		glVertexArrayAttribFormat(vao, MESH_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(vao, MESH_ATTRIB_POSITION, positionsBindingIndex);

		glEnableVertexArrayAttrib(vao, MESH_ATTRIB_TEX_COORD);
		glVertexArrayAttribFormat(vao, MESH_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(vao, MESH_ATTRIB_TEX_COORD, texCoordsBindingIndex);

		Model quad;
		quad.vao = vao;
		quad.indexCount = indexCount;
		quad.indexType = indexType;

		models.emplace_back(quad);
	}
	*/


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
				model.Draw();
			}
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
}
