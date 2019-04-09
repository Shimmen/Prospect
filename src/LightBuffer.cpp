#include "LightBuffer.h"

#include "Logging.h"
#include "TextureSystem.h"

#include "shader_locations.h"

void
LightBuffer::RecreateGpuResources(int width, int height, const GBuffer& gBuffer)
{
	this->width = width;
	this->height = height;

	// Docs: "glDeleteTextures silently ignores 0's and names that do not correspond to existing textures."
	glDeleteTextures(1, &lightTexture);
	lightTexture = TextureSystem::CreateTexture(width, height, GL_RGBA32F, GL_NEAREST, GL_NEAREST);

	if (!framebuffer)
	{
		glCreateFramebuffers(1, &framebuffer);
		glNamedFramebufferDrawBuffer(framebuffer, PredefinedOutputLocation(o_color));
	}

	glNamedFramebufferTexture(framebuffer, PredefinedOutputLocation(o_color), lightTexture, 0);

	// Use the g-buffer depth as the depth in the light pass (for reading only)
	glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, gBuffer.depthTexture, 0);

	GLenum status = glCheckNamedFramebufferStatus(framebuffer, GL_DRAW_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		Log("The Light buffer framebuffer is not complete!");
	}
}
