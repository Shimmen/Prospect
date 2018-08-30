#include "SkyPass.h"

#include "ShaderSystem.h"

#include "shader_locations.h"

void
SkyPass::Draw(const LightBuffer& lightBuffer, Scene& scene)
{
	if (!emptyVertexArray)
	{
		glCreateVertexArrays(1, &emptyVertexArray);
	}

	if (!skyProgram)
	{
		skyProgram = ShaderSystem::AddProgram("light/skybox.vert.glsl", "light/skybox.frag.glsl");
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_EQUAL);
	glDepthMask(GL_FALSE);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightBuffer.framebuffer);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(*skyProgram);
	{
		glBindTextureUnit(10, scene.skyTexture);
		glProgramUniform1i(*skyProgram, PredefinedUniformLocation(u_texture), 10);

		glBindVertexArray(emptyVertexArray);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
}