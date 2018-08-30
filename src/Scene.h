#pragma once

#include <vector>

#include <glad/glad.h>

#include "FpsCamera.h"
#include "Model.h"

struct DirectionalLight;

struct Scene
{
	FpsCamera mainCamera;

	GLuint skyTexture;

	std::vector<Model> models;

	std::vector<DirectionalLight> directionalLights;
};
