#pragma once

#include <glad/glad.h>

#include "ShaderSystem.h"
#include "TransformSystem.h"

struct Material: public ShaderDepandant
{
	bool opaque = true;
	bool cullBackfaces = true;

	GLuint program = 0;
	virtual void ProgramLoaded(GLuint program) = 0;

	// Call before drawing with material
	virtual void BindUniforms(Transform& transform, const Transform& prevTransform) const = 0;
};
