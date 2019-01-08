#include "ShaderSystem.h"

#include <fstream>
#include <sstream>
#include <functional>
#include <unordered_set>

#ifdef _WIN32
 #include <sys/stat.h>
#endif

#include "Logging.h"

//
// Internal data structures
//

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

	bool operator==(const Program& other) const
	{
		return fixedLocation == other.fixedLocation;
	}
};

template <>
struct std::hash<Program>
{
	std::size_t operator()(const Program& program) const
	{
		return 17 * program.fixedLocation;
	}
};

struct GlslFile
{
	std::string filename;
	uint64_t timestamp = 0;
	std::unordered_set<Program> dependablePrograms{};

	GlslFile() {}
	explicit GlslFile(const std::string& filename) : filename(filename) {}
};

//
// Data
//

// TODO: Make this setable!
std::string shaderDirectory{ "shaders/" };

std::unordered_map<std::string, GlslFile> managedFiles;

// Maps from a program name to an index into the publicProgramHandles array
std::unordered_map<std::string, size_t> managedPrograms{};

// Maps from a program location to a list of material ID's that need to be reinit on program changes
std::unordered_map<size_t, std::unordered_set<ShaderDepandant *>> dependantObjects{};

// It is very important that the memory below is never moved or reordered!
// External pointers point to elements in in this array
std::array<GLuint, MAX_NUM_MANAGED_PROGRAMS> publicProgramHandles{};

size_t nextPublicHandleIndex = 0;

//
// Internal API
//

void
AddManagedFile(const std::string& filename, const Program& dependableProgram)
{
	if (managedFiles.find(filename) == managedFiles.end())
	{
		managedFiles[filename] = GlslFile(filename);
	}

	managedFiles[filename].dependablePrograms.emplace(dependableProgram);
}

uint64_t
GetTimestamp(const GlslFile& file)
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

bool
FileReadable(const std::string& filename)
{
	auto path = shaderDirectory + filename;
	std::ifstream ifs(path);
	return ifs.good();
}

void
ReadFileWithIncludes(const std::string& filename, const Program& dependableProgram, std::stringstream& sourceBuffer)
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
		size_t commentIndex = line.find("//");
		size_t index = line.find("#include");

		if (index == -1 || (commentIndex < index && commentIndex != -1))
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

void
UpdateProgram(Program& program)
{
	static GLchar statusBuffer[4096];
	bool oneOrMoreShadersFailedToCompile = false;

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
			oneOrMoreShadersFailedToCompile = true;
			glGetShaderInfoLog(shaderHandle, sizeof(statusBuffer), nullptr, statusBuffer);
			Log("Shader compilation error ('%s'): %s\n", shader.filename.c_str(), statusBuffer);

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

	if (oneOrMoreShadersFailedToCompile)
	{
		return;
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

		// Notify all dependant objects
		for (auto shaderDependant : dependantObjects[program.fixedLocation])
		{
			if (shaderDependant)
			{
				shaderDependant->ProgramLoaded(programHandle);
			}
		}
	}
}

//
// Public API
//

void
ShaderSystem::Update()
{
	static GLchar statusBuffer[4096];
	std::unordered_set<Program> programsToUpdate{};

	for (auto& pair : managedFiles)
	{
		GlslFile& file = pair.second;

		uint64_t timestamp = GetTimestamp(file);
		if (timestamp != file.timestamp)
		{
			file.timestamp = timestamp;

			// Add all programs that are dependant of this file (directly or included)
			programsToUpdate.insert(file.dependablePrograms.begin(), file.dependablePrograms.end());
		}
	}

	for (auto program : programsToUpdate)
	{
		UpdateProgram(program);
	}
}

GLuint *
ShaderSystem::AddProgram(const std::string& name, ShaderDepandant *shaderDependant)
{
	return AddProgram(name + ".vert.glsl", name + ".frag.glsl", shaderDependant);
}

GLuint *
ShaderSystem::AddProgram(const std::string& vertName, const std::string& fragName, ShaderDepandant *shaderDependant)
{
	std::string fullName = vertName + "_" + fragName;
	if (managedPrograms.find(fullName) == managedPrograms.end())
	{
		Program program;
		program.fixedLocation = nextPublicHandleIndex++;

		Shader vertexShader(GL_VERTEX_SHADER, vertName);
		program.shaders.push_back(vertexShader);

		// Since there can be programs without fragment shaders, consider it optional
		bool includeFragShader = FileReadable(fragName);
		if (includeFragShader)
		{
			Shader fragmentShader(GL_FRAGMENT_SHADER, fragName);
			program.shaders.push_back(fragmentShader);
		}

		AddManagedFile(vertName, program);
		if (includeFragShader) AddManagedFile(fragName, program);

		if (shaderDependant)
		{
			dependantObjects[program.fixedLocation].emplace(shaderDependant);
		}

		// Trigger the initial load
		UpdateProgram(program);

		managedPrograms[fullName] = program.fixedLocation;
		return &publicProgramHandles[program.fixedLocation];
	}
	else
	{
		// If this exact program is already added (i.e. same vert & frag names), return that address instead
		size_t fixedLocation = managedPrograms[fullName];

		if (shaderDependant)
		{
			dependantObjects[fixedLocation].emplace(shaderDependant);

			// Since this exact program is added previously there is a chance that it's already loaded.
			// If it is, call program loaded immediately so that it can perform its initial setup.
			GLuint program = publicProgramHandles[fixedLocation];
			if (program)
			{
				shaderDependant->ProgramLoaded(program);
			}
			
		}

		return &publicProgramHandles[fixedLocation];
	}

}

void
ShaderSystem::AddProgram(GLuint** programOut, const std::string& vertName, const std::string& fragName, ShaderDepandant *shaderDependant)
{
	std::string fullName = vertName + "_" + fragName;
	if (managedPrograms.find(fullName) == managedPrograms.end())
	{
		Program program;
		program.fixedLocation = nextPublicHandleIndex++;

		Shader vertexShader(GL_VERTEX_SHADER, vertName);
		program.shaders.push_back(vertexShader);

		// Since there can be programs without fragment shaders, consider it optional
		bool includeFragShader = FileReadable(fragName);
		if (includeFragShader)
		{
			Shader fragmentShader(GL_FRAGMENT_SHADER, fragName);
			program.shaders.push_back(fragmentShader);
		}

		AddManagedFile(vertName, program);
		if (includeFragShader) AddManagedFile(fragName, program);

		if (shaderDependant)
		{
			dependantObjects[program.fixedLocation].emplace(shaderDependant);
		}

		managedPrograms[fullName] = program.fixedLocation;
		*programOut = &publicProgramHandles[program.fixedLocation];

		// Trigger the initial load
		UpdateProgram(program);
	}
	else
	{
		// If this exact program is already added (i.e. same vert & frag names), return that address instead
		size_t fixedLocation = managedPrograms[fullName];
		*programOut = &publicProgramHandles[fixedLocation];

		if (shaderDependant)
		{
			dependantObjects[fixedLocation].emplace(shaderDependant);

			// Since this exact program is added previously there is a chance that it's already loaded.
			// If it is, call program loaded immediately so that it can perform its initial setup.
			GLuint program = publicProgramHandles[fixedLocation];
			if (program)
			{
				shaderDependant->ProgramLoaded(program);
			}

		}
	}
}

void
ShaderSystem::AddComputeProgram(GLuint** programOut, const std::string& name, ShaderDepandant *shaderDependant)
{
	if (managedPrograms.find(name) == managedPrograms.end())
	{
		Program program;
		program.fixedLocation = nextPublicHandleIndex++;

		Shader shader(GL_COMPUTE_SHADER, name);
		program.shaders.push_back(shader);

		AddManagedFile(name, program);

		if (shaderDependant)
		{
			dependantObjects[program.fixedLocation].emplace(shaderDependant);
		}

		managedPrograms[name] = program.fixedLocation;
		*programOut = &publicProgramHandles[program.fixedLocation];

		// Trigger the initial load
		UpdateProgram(program);
	}
	else
	{
		size_t fixedLocation = managedPrograms[name];
		*programOut = &publicProgramHandles[fixedLocation];

		if (shaderDependant)
		{
			dependantObjects[fixedLocation].emplace(shaderDependant);

			// Since this exact program is added previously there is a chance that it's already loaded.
			// If it is, call program loaded immediately so that it can perform its initial setup.
			GLuint program = publicProgramHandles[fixedLocation];
			if (program)
			{
				shaderDependant->ProgramLoaded(program);
			}
		}
	}
}

GLuint *
ShaderSystem::AddComputeProgram(const std::string& name, ShaderDepandant* shaderDependant)
{
	if (managedPrograms.find(name) == managedPrograms.end())
	{
		Program program;
		program.fixedLocation = nextPublicHandleIndex++;

		Shader shader(GL_COMPUTE_SHADER, name);
		program.shaders.push_back(shader);

		AddManagedFile(name, program);

		if (shaderDependant)
		{
			dependantObjects[program.fixedLocation].emplace(shaderDependant);
		}

		// Trigger the initial load
		UpdateProgram(program);

		managedPrograms[name] = program.fixedLocation;
		return &publicProgramHandles[program.fixedLocation];
	}
	else
	{
		size_t fixedLocation = managedPrograms[name];

		if (shaderDependant)
		{
			dependantObjects[fixedLocation].emplace(shaderDependant);

			// Since this exact program is added previously there is a chance that it's already loaded.
			// If it is, call program loaded immediately so that it can perform its initial setup.
			GLuint program = publicProgramHandles[fixedLocation];
			if (program)
			{
				shaderDependant->ProgramLoaded(program);
			}
		}

		return &publicProgramHandles[fixedLocation];
	}
}
