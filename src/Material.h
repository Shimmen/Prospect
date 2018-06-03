#pragma once

#include <glad/glad.h>

#include "ShaderSystem.h"

struct Material
{
	GLuint *program;

	// Is called by the MaterialSystem on adding and readding this material
	virtual void Init(int materialID) = 0;

	// Call before drawing with material
	virtual void BindUniforms() const = 0;
};
