#include "LightPass.h"

#include "TextureSystem.h"

#include "shader_locations.h"

GLuint *blitProgram = nullptr;
GLuint blitTexture = 0;
GLint textureUnit = 0;

void
LightPass::Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, FpsCamera& camera)
{
	if (!emptyVertexArray)
	{
		glCreateVertexArrays(1, &emptyVertexArray);
	}

	if (!blitProgram)
	{
		blitProgram = ShaderSystem::AddProgram("quad.vert.glsl", "blit.frag.glsl");
		glProgramUniform1i(*blitProgram, PredefinedUniformLocation(u_texture), textureUnit);
	}

	if (!blitTexture)
	{
		blitTexture = TextureSystem::LoadLdrImage("assets/images/default.png");
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightBuffer.framebuffer);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(*blitProgram);
	{
		glBindTextureUnit(textureUnit, blitTexture);

		glDisable(GL_DEPTH_TEST);

		glBindVertexArray(emptyVertexArray);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_DEPTH_TEST);
	}
}