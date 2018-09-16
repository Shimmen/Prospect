#include "IBLPass.h"

#include <imgui.h>

#include "Scene.h"
#include "GuiSystem.h"
#include "TextureSystem.h"

using namespace glm;
#include "shader_locations.h"
#include "shader_types.h"

void
IBLPass::Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, Scene& scene)
{
	if (!emptyVertexArray)
	{
		glCreateVertexArrays(1, &emptyVertexArray);
	}

	glBindTextureUnit(0, gBuffer.albedoTexture);
	glBindTextureUnit(1, gBuffer.normalTexture);
	glBindTextureUnit(2, gBuffer.depthTexture);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightBuffer.framebuffer);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	if (indirectLight)
	{
		if (!indirectIBLProgram)
		{
			indirectIBLProgram = ShaderSystem::AddProgram("light/ibl.vert.glsl", "light/iblIndirect.frag.glsl");
			glProgramUniform1i(*indirectIBLProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
			glProgramUniform1i(*indirectIBLProgram, PredefinedUniformLocation(u_g_buffer_normal), 1);
			glProgramUniform1i(*indirectIBLProgram, PredefinedUniformLocation(u_g_buffer_depth), 2);

			GLint loc = glGetUniformLocation(*indirectIBLProgram, "u_irradiance");
			glProgramUniform1i(*indirectIBLProgram, loc, 3);
		}

		// TODO: Fix error where we need to set this each frame..
		glProgramUniform1i(*indirectIBLProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
		glProgramUniform1i(*indirectIBLProgram, PredefinedUniformLocation(u_g_buffer_normal), 1);
		glProgramUniform1i(*indirectIBLProgram, PredefinedUniformLocation(u_g_buffer_depth), 2);

		glUseProgram(*indirectIBLProgram);
		glDisable(GL_DEPTH_TEST);

		GLint loc = glGetUniformLocation(*indirectIBLProgram, "u_irradiance");
		glProgramUniform1i(*indirectIBLProgram, loc, 3);
		glBindTextureUnit(3, scene.skyIrradiance);

		glBindVertexArray(emptyVertexArray);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_DEPTH_TEST);
	}

	if (ImGui::CollapsingHeader("IBL settings"))
	{
		ImGui::Checkbox("Direct light", &directLight);
		ImGui::Checkbox("Indirect light", &indirectLight);
	}
}