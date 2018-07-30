#pragma once

#include "Material.h"

struct Lambertian: public Material
{
	Lambertian();

	GLuint diffuseTexture;
	GLint diffuseTextureLocation;

	GLint modelMatrixLocation;

	virtual void ProgramLoaded(GLuint program) override;
	virtual void BindUniforms(Transform& transform) const override;
};
