#pragma once

#include <glad/glad.h>

struct GBuffer
{
	int width;
	int height;

	GLuint framebuffer;

	// RGBA8: RGB - albedo, A - unused (TODO: emissive?)
	GLuint albedoTexture = 0;

	// RGBA8: R - metallic, G - roughness, BA - unused
	GLuint materialTexture = 0;

	// TODO: Compress normals to RG only (with maybe octahedral mapping?) and put xy-velocity in BA
	// RGBA8: RGB - normal xyz, A - unused
	GLuint normalTexture = 0;

	// DEPTH_COMPONENT32: projected non-linear depth
	GLuint depthTexture = 0;

	////////////////////////////

	// Create or recreate the g-buffer textures with the given dimensions
	void RecreateGpuResources(int width, int height);

};
