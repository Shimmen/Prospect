#pragma once

#include <glad/glad.h>

//#include "LightPass.h"

struct ShadowMap
{
	int size;

	GLuint framebuffer;

	// DEPTH_COMPONENT_32F
	GLuint texture = 0;

	////////////////////////////

	void RecreateGpuResources(int size);

	//void AssignShadowMapSegments(std::vector<DirectionalLight>& dirLights/* TODO: Add the other lights here as well!*/);

};
