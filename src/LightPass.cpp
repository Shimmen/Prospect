#include "LightPass.h"

#include "TextureSystem.h"

#include "shader_locations.h"

void
LightPass::Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, const ShadowMap& shadowMap, FpsCamera& camera)
{
	if (!emptyVertexArray)
	{
		glCreateVertexArrays(1, &emptyVertexArray);
	}

	if (!directionalLightProgram)
	{
		directionalLightProgram = ShaderSystem::AddProgram("light/directional.vert.glsl", "light/directional.frag.glsl");

		glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
		glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_g_buffer_normal), 1);
		glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_g_buffer_depth), 2);
	}

	// TODO: We shouldn't need to do this every loop, right?
	glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
	glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_g_buffer_normal), 1);
	glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_g_buffer_depth), 2);

	glBindTextureUnit(0, gBuffer.albedoTexture);
	glBindTextureUnit(1, gBuffer.normalTexture);
	glBindTextureUnit(2, gBuffer.depthTexture);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightBuffer.framebuffer);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glBindTextureUnit(10, shadowMap.texture);
	glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_shadow_map), 10);

	glUseProgram(*directionalLightProgram);
	{
		glDisable(GL_DEPTH_TEST);

		glBindVertexArray(emptyVertexArray);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_DEPTH_TEST);
	}
}