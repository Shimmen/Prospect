#pragma once

#include <glad/glad.h>

#include "ShaderSystem.h"
#include "TransformSystem.h"

struct Material
{
	GLuint *program;

	// Is called by the MaterialSystem on adding and reading this material, and when the program is reloaded
	virtual void Init(int thisMaterialID) = 0;

	// Call before drawing with material
	virtual void BindUniforms(Transform& transform) const = 0;
};
