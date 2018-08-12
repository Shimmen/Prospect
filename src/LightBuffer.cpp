#include "LightBuffer.h"

#include "Logging.h"

#include "shader_locations.h"

void
LightBuffer::RecreateGpuResources(int width, int height)
{
	this->width = width;
	this->height = height;

	// Docs: "glDeleteTextures silently ignores 0's and names that do not correspond to existing textures."
	glDeleteTextures(1, &lightTexture);

	glCreateTextures(GL_TEXTURE_2D, 1, &lightTexture);
	glTextureStorage2D(lightTexture, 1, GL_RGB32F, width, height);

	glTextureParameteri(lightTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(lightTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(lightTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(lightTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (!framebuffer)
	{
		glCreateFramebuffers(1, &framebuffer);
		glNamedFramebufferDrawBuffer(framebuffer, PredefinedOutputLocation(o_color));
	}

	glNamedFramebufferTexture(framebuffer, PredefinedOutputLocation(o_color), lightTexture, 0);

	GLenum status = glCheckNamedFramebufferStatus(framebuffer, GL_DRAW_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		LogError("The Light buffer framebuffer is not complete!");
	}
}