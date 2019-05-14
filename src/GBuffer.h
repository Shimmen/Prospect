#pragma once

#include <glad/glad.h>

struct GBuffer
{
	int width;
	int height;

	GLuint framebuffer;

	// RGBA8: RGB - albedo, A - unused (TODO: emissive?)
	GLuint albedoTexture = 0;

	// RGBA8: R - roughness, G - metallic, BA - unused
	GLuint materialTexture = 0;

	// RGBA16F: RG - octahedral mapped normal, BA - screen space velocity
	GLuint normVelTexture = 0;

	// DEPTH_COMPONENT32: projected non-linear depth
	GLuint depthTexture = 0;

	////////////////////////////

	// Create or recreate the g-buffer textures with the given dimensions
	void RecreateGpuResources(int width, int height);

};
