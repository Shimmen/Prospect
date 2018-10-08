#pragma once

#include "Material.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

struct CompleteMaterial : public Material
{
	CompleteMaterial();
	~CompleteMaterial() = default;

	// TODO: Replace these with textures later!
	float roughness;
	float metallic;

	mutable GLuint baseColorTexture{};
	mutable GLuint normalMap{};

	virtual void ProgramLoaded(GLuint program) override;
	virtual void BindUniforms(Transform& transform) const override;

private:

	GLint baseColorLocation;
	GLint roughnessLocation;
	GLint metallicLocation;
	GLint normalMapLocation;

	GLint modelMatrixLocation;
	GLint normalMatrixLocation;

};
