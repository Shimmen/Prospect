#pragma once

#include <glad/glad.h>

#include "ShaderSystem.h"

struct Material
{
	GLuint* shader;

	virtual void Init(ShaderSystem& shaderSystem) = 0;
	virtual void BindUniforms() const = 0;
};
