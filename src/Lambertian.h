#pragma once

#include "Material.h"

struct Lambertian: public Material
{
	GLuint diffuseTexture;
	GLint diffuseTextureLocation;

	virtual void Init() override;
	virtual void BindUniforms() const override;
};
