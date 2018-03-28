#include "ShaderSystem.h"

#include <fstream>
#include <sstream>

#ifdef _WIN32
 #include <sys/stat.h>
#endif

#include "Logging.h"

ShaderSystem::ShaderSystem(const std::string& shaderDirectory)
	: shaderDirectory(shaderDirectory)
{
}

void
ShaderSystem::Update()
{
	static GLchar statusBuffer[4096];
	std::vector<Program> programsToUpdate{};

	for (auto& pair : managedFiles)
	{
		GlslFile& file = pair.second;

		uint64_t timestamp = GetTimestamp(file);
		if (timestamp != file.timestamp)
		{
			file.timestamp = timestamp;

			// TODO TODO TODO TODO TODO TODO
			// TODO  Avoid duplicates!  TOOD
			// TODO TODO TODO TODO TODO TODO
			programsToUpdate.insert(programsToUpdate.end(),
				file.dependablePrograms.begin(), file.dependablePrograms.end());
		}
	}

	for (auto& program : programsToUpdate)
	{
		GLuint programHandle = glCreateProgram();
		std::vector<GLuint> shaderHandles{};

		for (auto& shader : program.shaders)
		{
			std::stringstream sourceBuffer{};
			ReadFileWithIncludes(shader.filename, program, sourceBuffer);

			GLuint shaderHandle = glCreateShader(shader.type);

			std::string source = sourceBuffer.str();
			const GLchar* sources[] = { source.c_str() };
			glShaderSource(shaderHandle, 1, sources, nullptr);
			glCompileShader(shaderHandle);

			GLint compilationSuccess;
			glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compilationSuccess);
			if (compilationSuccess != GL_TRUE)
			{
				glGetShaderInfoLog(shaderHandle, sizeof(statusBuffer), nullptr, statusBuffer);
				Log("Shader compilation error ('%s'): %s", shader.filename.c_str(), statusBuffer);

				int lineNum = 1;
				std::istringstream iss(source);
				for (std::string line; std::getline(iss, line); ++lineNum)
				{
					Log(" %d: %s\n", lineNum, line.c_str());
				}
				
			}
			else
			{
				glAttachShader(programHandle, shaderHandle);
				shaderHandles.push_back(shaderHandle);
			}
		}

		glLinkProgram(programHandle);

		// (it's safe to detach and delete shaders after linking)
		for (GLuint shaderHandle : shaderHandles)
		{
			glDetachShader(programHandle, shaderHandle);
			glDeleteShader(shaderHandle);
		}

		GLint linkSuccess;
		glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
		if (linkSuccess != GL_TRUE)
		{
			glGetProgramInfoLog(programHandle, sizeof(statusBuffer), nullptr, statusBuffer);
			Log("Shader program link error: %s\n", statusBuffer);
		}
		else
		{
			// The program successfully compiled and linked, it's safe to replace the old one

			size_t index = program.fixedLocation;

			GLuint oldProgramHandle = publicProgramHandles[index];
			if (oldProgramHandle)
			{
				glDeleteProgram(oldProgramHandle);
			}

			publicProgramHandles[index] = programHandle;
		}
	}
}

GLuint*
ShaderSystem::AddProgram(const std::string& name)
{
	return AddProgram(name + ".vert.glsl", name + ".frag.glsl");
}

GLuint*
ShaderSystem::AddProgram(const std::string& vertName, const std::string& fragName)
{
	Shader vertexShader(GL_VERTEX_SHADER, vertName);
	Shader fragmentShader(GL_FRAGMENT_SHADER, fragName);

	Program program;
	program.fixedLocation = nextPublicHandleIndex++;
	program.shaders.push_back(vertexShader);
	program.shaders.push_back(fragmentShader);

	AddManagedFile(vertName, program);
	AddManagedFile(fragName, program);

	return &publicProgramHandles[program.fixedLocation];
}

void
ShaderSystem::AddManagedFile(const std::string& filename, const Program& dependableProgram)
{
	if (managedFiles.find(filename) == managedFiles.end())
	{
		managedFiles[filename] = GlslFile(filename);
	}

	// TODO TODO TODO TODO TODO TODO
	// TODO  Avoid duplicates!  TOOD
	// TODO TODO TODO TODO TODO TODO
	managedFiles[filename].dependablePrograms.push_back(dependableProgram);
}

uint64_t
ShaderSystem::GetTimestamp(const GlslFile& file) const
{
	auto filename = shaderDirectory + file.filename;
	uint64_t timestamp = 0;

#ifdef _WIN32
	struct __stat64 stFileInfo;
	if (_stat64(filename.c_str(), &stFileInfo) == 0)
	{
		timestamp = stFileInfo.st_mtime;
	}
#else
 #warning "ShaderLoader doesn't currently support anything other than Windows for shader reloading!"
#endif

	return timestamp;
}

void
ShaderSystem::ReadFileWithIncludes(const std::string& filename, const Program& dependableProgram, std::stringstream& sourceBuffer)
{
	// (won't do anything if the file is already managed)
	AddManagedFile(filename, dependableProgram);

	auto path = shaderDirectory + filename;
	std::ifstream ifs(path);
	if (!ifs.good())
	{
		Log("Could not read shader file '%s'.\n", filename.c_str());
	}

	std::string line;
	while (std::getline(ifs, line))
	{
		size_t index = line.find("#include");

		if (index == -1)
		{
			sourceBuffer << line;
		}
		else
		{
			// This isn't very precise but it should work assuming that
			// the programmer isn't doing anything really stupid.
			size_t start = line.find('<') + 1;
			size_t end = line.find('>');
			
			size_t count = end - start;
			if (count < 0)
			{
				LogError("Invalid include directive: %s\n", line.c_str());
			}

			std::string includeFile = line.substr(start, count);
			ReadFileWithIncludes(includeFile, dependableProgram, sourceBuffer);
		}

		sourceBuffer << '\n';
	}
}
