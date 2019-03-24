#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace prospect
{
	namespace internal
	{
		template<typename T>
		void PerformUniformUpdate(GLuint program, GLint location, T value)
		{
			assert(false);
		}

		template <>
		inline void PerformUniformUpdate(GLuint program, GLint location, float value)
		{
			glProgramUniform1f(program, location, value);
		}

		template <>
		inline void PerformUniformUpdate(GLuint program, GLint location, glm::vec2 value)
		{
			glProgramUniform2fv(program, location, 1, &value.x);
		}

		template <>
		inline void PerformUniformUpdate(GLuint program, GLint location, glm::vec3 value)
		{
			glProgramUniform3fv(program, location, 1, &value.x);
		}

		template <>
		inline void PerformUniformUpdate(GLuint program, GLint location, glm::vec4 value)
		{
			glProgramUniform4fv(program, location, 1, &value.x);
		}
	}
}

template<typename T>
struct Uniform
{
	Uniform(const char *name, T initialValue)
	{
		this->name = name;
		this->value = initialValue;
	}

	Uniform(Uniform& other) = delete;
	Uniform(Uniform&& other) = delete;
	Uniform& operator=(Uniform& other) = delete;
	Uniform& operator=(Uniform&& other) = delete;

	~Uniform() = default;

	void UpdateUniformIfNeeded(GLuint program)
	{
		if (location == 0 || program != lastProgram)
		{
			location = glGetUniformLocation(program, name);
		}

		if (value != lastValue || program != lastProgram)
		{
			prospect::internal::PerformUniformUpdate(program, location, value);
			lastValue = value;
		}

		lastProgram = program;
	}

	GLint location = 0;
	float value = -1234.5678f;

private:

	const char *name;

	GLuint lastProgram = 0;
	T lastValue = T{};
	
};
