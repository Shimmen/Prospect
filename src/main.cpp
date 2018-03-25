#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <string>
#include <sstream>
#include <fstream>
#include <cassert>
#include <stdlib.h>


//
// Callbacks
//

void gl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	printf("GL debug message: %s\n", message);
}

void glfw_error_callback(int code, const char *message)
{
    printf("GLFW error %d: %s\n", code, message);
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

GLuint createShader(const char *filePath, GLenum type)
{
    std::ifstream ifs(filePath);
    if (!ifs.good()) {
        printf("Could not read shader file '%s'. Aborting.\n", filePath);
        exit(EXIT_FAILURE);
    }
    
    std::stringstream buffer;
    buffer << ifs.rdbuf();
	std::string source = buffer.str();
    const GLchar *sources[] = { source.c_str() };
    
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, sources, nullptr);
    glCompileShader(shader);

    GLint compilationSuccess;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compilationSuccess);
    if (compilationSuccess != GL_TRUE) {
        static GLchar errorMessage[2048];
        glGetShaderInfoLog(shader, sizeof(errorMessage), nullptr, errorMessage);
        printf("Shader compilation error ('%s'): %s. Aborting.\n", filePath, errorMessage);
        exit(EXIT_FAILURE);
    }

    return shader;
}

GLuint createProgram(GLuint vertexShader, GLuint fragmentShader = 0, GLuint geometryShader = 0)
{
    GLuint program = glCreateProgram();
    if (vertexShader) glAttachShader(program, vertexShader);
    if (fragmentShader) glAttachShader(program, fragmentShader);
    if (geometryShader) glAttachShader(program, geometryShader);
    glLinkProgram(program);

    GLint linkSuccess;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess != GL_TRUE) {
        static GLchar errorMessage[2048];
        glGetProgramInfoLog(program, sizeof(errorMessage), nullptr, errorMessage);
        printf("Shader program link error: %s. Aborting.\n", errorMessage);
        exit(EXIT_FAILURE);
    }

    return program;
}

GLuint loadImage(const char *filePath)
{
    stbi_set_flip_vertically_on_load(true);

    int width, height, numComponents;
    unsigned char *pixels = stbi_load(filePath, &width, &height, &numComponents, 0);

    if (pixels == nullptr) {
        printf("Image loading error ('%s'): %s. Aborting.\n", filePath, stbi_failure_reason());
        exit(EXIT_FAILURE);
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
            printf("Image loading error: unknown number of components (%d). Aborting.", numComponents);
            exit(EXIT_FAILURE);
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
    GLuint program = createProgram(
            createShader("shaders/default.vert.glsl", GL_VERTEX_SHADER),
            createShader("shaders/default.frag.glsl", GL_FRAGMENT_SHADER));
    glUseProgram(program);

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
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

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
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindVertexArray(0);

        glDeleteBuffers(1, &positionsBuffer);
        glDeleteBuffers(1, &uvBuffer);
    }


    // Render loop
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, testScreen);
            glUniform1i(glGetUniformLocation(program, "u_texture"), 0);

            glBindVertexArray(quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
}
