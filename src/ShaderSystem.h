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
class ShaderSystem
{
public:

	explicit ShaderSystem(const std::string& shaderDirectory);
	~ShaderSystem() = default;

	ShaderSystem(ShaderSystem& other) = delete;
	ShaderSystem& operator=(ShaderSystem& other) = delete;

	// Reload recompile and relink shaders/programs if needed. Call this every
	// render loop iteration before using any shaders managed by this shader loader.
	void Update();

	// Add a shader program with the specified file name (*.vert.glsl and *.frag.glsl assumed)
	GLuint* AddProgram(const std::string& name);

	// Add a shader program with the specified names for the vertex and fragment shaders
	GLuint* AddProgram(const std::string& vertName, const std::string& fragName);

private:

	struct Shader
	{
		GLenum type;
		std::string filename;

		Shader(GLenum type, const std::string& filename) : type(type), filename(filename) {}
	};

	struct Program
	{
		size_t fixedLocation;
		std::vector<Shader> shaders{};
	};

	struct GlslFile
	{
		std::string filename;
		uint64_t timestamp = 0;
		std::vector<Program> dependablePrograms{};

		GlslFile() {}
		explicit GlslFile(const std::string& filename) : filename(filename) {}
	};

	void AddManagedFile(const std::string& filename, const Program& dependableProgram);
	uint64_t GetTimestamp(const GlslFile& file) const;
	void ReadFileWithIncludes(const std::string& filename, const Program& dependableProgram, std::stringstream& sourceBuffer);

	std::string shaderDirectory;
	std::unordered_map<std::string, GlslFile> managedFiles;

	// It is very important that the memory below is never moved or reordered!
	// External pointers point to elements in in this array
	std::array<GLuint, MAX_NUM_MANAGED_PROGRAMS> publicProgramHandles{};

	size_t nextPublicHandleIndex = 0;

};
