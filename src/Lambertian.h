#pragma once

#include "Material.h"

struct Lambertian: public Material
{
	GLuint diffuseTexture;
	GLint diffuseTextureLocation;

	GLint modelMatrixLocation;

	virtual void Init(int thisMaterialID) override;
	virtual void BindUniforms(Transform& transform) const override;
};
