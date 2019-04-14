#pragma once

#include <vector>

#include <glad/glad.h>

#include "CameraBase.h"
#include "Model.h"

#include "shader_types.h"

struct DirectionalLight;

struct Probe
{
	// Incoming radiance to the probe (i.e. skybox)
	GLuint radiance{};

	// Filtered radiance, where every MIP corresponds to a roughness
	GLuint filteredRadiance{};

	// Irradiance, filtered for the Lambert diffuse brdf
	GLuint diffuseIrradianceSh{};
};

struct Scene
{
	std::unique_ptr<CameraBase> mainCamera;

	Probe skyProbe;

	std::vector<Model> models;

	std::vector<DirectionalLight> directionalLights;
};
