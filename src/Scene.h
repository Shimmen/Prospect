#pragma once

#include <vector>

#include "FpsCamera.h"
#include "Model.h"

struct DirectionalLight;

struct Scene
{
	FpsCamera mainCamera;

	std::vector<Model> models;

	std::vector<DirectionalLight> directionalLights;
};
