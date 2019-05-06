#pragma once

#include <glad/glad.h>

#include <string>
#include <vector>

#include "ShaderDependant.h"

// Defines how many programs that the shader loader can manage. Should be set at a safe maximum.
#ifndef MAX_NUM_MANAGED_PROGRAMS
 #define MAX_NUM_MANAGED_PROGRAMS 128
#endif

struct ShaderErrorReport
{
	std::string shaderName;
	std::string errorMessage;;
	std::string preprocessedSource;
};

//
// A shader live-reloading system. Idea heavily inspired by Nicolas Guillemot's ShaderSet:
//   https://github.com/nlguillemot/ShaderSet
//
namespace ShaderSystem
{
	// Reload recompile and relink shaders/programs if needed. Call this every
	// render loop iteration before using any shaders managed by this shader loader.
	void Update();

	std::vector<ShaderErrorReport> GetShaderErrorReports();

	//

	// Add a shader program with the specified file name (*.vert.glsl and *.frag.glsl assumed)
	// Supply a shader depandant if there is some object that should be updated with this program
	GLuint* AddProgram(const std::string& name, ShaderDepandant *shaderDependant = nullptr);

	// Add a shader program with the specified names for the vertex and fragment shaders
	// Supply a shader depandant if there is some object that should be updated with this program
	GLuint* AddProgram(const std::string& vertName, const std::string& fragName, ShaderDepandant *shaderDependant = nullptr);

	void AddProgram(GLuint** programOut, const std::string& vertName, const std::string& fragName, ShaderDepandant *shaderDependant = nullptr);
	void AddComputeProgram(GLuint** programOut, const std::string& name, ShaderDepandant *shaderDependant = nullptr);

	GLuint* AddComputeProgram(const std::string& name, ShaderDepandant *shaderDependant = nullptr);
}
