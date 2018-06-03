#pragma once

#include <glad/glad.h>

#include <unordered_map>
#include <stdint.h>
#include <vector>
#include <string>
#include <array>

// Defines how many programs that the shader loader can manage. Should be set at a safe maximum.
#ifndef MAX_NUM_MANAGED_PROGRAMS
 #define MAX_NUM_MANAGED_PROGRAMS 128
#endif

//
// A shader live-reloading system. Idea heavily inspired by Nicolas Guillemot's ShaderSet:
//   https://github.com/nlguillemot/ShaderSet
//
namespace ShaderSystem
{
	// Reload recompile and relink shaders/programs if needed. Call this every
	// render loop iteration before using any shaders managed by this shader loader.
	void Update();

	// Set the base path for all shader file loading 
	// TODO: Make sure this supports changing the base path!
	//void SetBasePath(const std::string& path);

	enum { NO_MATERIAL = -1 };

	// Add a shader program with the specified file name (*.vert.glsl and *.frag.glsl assumed)
	// Supply a material ID if there is a specific material that should be updated with this program
	GLuint* AddProgram(const std::string& name, int materialID = NO_MATERIAL);

	// Add a shader program with the specified names for the vertex and fragment shaders
	// Supply a material ID if there is a specific material that should be updated with this program
	GLuint* AddProgram(const std::string& vertName, const std::string& fragName, int materialID = NO_MATERIAL);
}
