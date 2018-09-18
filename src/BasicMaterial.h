#pragma once

#include "Material.h"

#include <glm/glm.hpp>

struct BasicMaterial : public Material
{
	BasicMaterial();
	~BasicMaterial() = default;

	glm::vec3 baseColor;
	float roughness;
	float metallic;

	virtual void ProgramLoaded(GLuint program) override;
	virtual void BindUniforms(Transform& transform) const override;

private:

	GLint baseColorLocation;
	GLint roughnessLocation;
	GLint metallicLocation;

	GLint modelMatrixLocation;
	GLint normalMatrixLocation;

};
