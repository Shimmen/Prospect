#pragma once

#include "Material.h"

struct Lambertian: public Material
{
	Lambertian();
	~Lambertian() = default;

	GLuint diffuseTexture;
	GLint diffuseTextureLocation;

	GLint modelMatrixLocation;
	GLint normalMatrixLocation;

	virtual void ProgramLoaded(GLuint program) override;
	virtual void BindUniforms(Transform& transform) const override;
};
