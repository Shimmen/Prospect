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

	//
	// TODO: Create brdf LUT for IBL radiance
	//

	glBindTextureUnit(0, gBuffer.albedoTexture);
	glBindTextureUnit(1, gBuffer.materialTexture);
	glBindTextureUnit(2, gBuffer.normalTexture);
	glBindTextureUnit(3, gBuffer.depthTexture);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightBuffer.framebuffer);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	if (indirectLight)
	{
		if (!iblProgram)
		{
			iblProgram = ShaderSystem::AddProgram("light/ibl.vert.glsl", "light/ibl.frag.glsl");
			glProgramUniform1i(*iblProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
			glProgramUniform1i(*iblProgram, PredefinedUniformLocation(u_g_buffer_material), 1);
			glProgramUniform1i(*iblProgram, PredefinedUniformLocation(u_g_buffer_normal), 2);
			glProgramUniform1i(*iblProgram, PredefinedUniformLocation(u_g_buffer_depth), 3);

			GLint loc = glGetUniformLocation(*iblProgram, "u_irradiance");
			glProgramUniform1i(*iblProgram, loc, 3);
		}

		// TODO: Fix error where we need to set this each frame..
		glProgramUniform1i(*iblProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
		glProgramUniform1i(*iblProgram, PredefinedUniformLocation(u_g_buffer_material), 1);
		glProgramUniform1i(*iblProgram, PredefinedUniformLocation(u_g_buffer_normal), 2);
		glProgramUniform1i(*iblProgram, PredefinedUniformLocation(u_g_buffer_depth), 3);

		glUseProgram(*iblProgram);
		glDisable(GL_DEPTH_TEST);

		GLint loc = glGetUniformLocation(*iblProgram, "u_irradiance");
		glProgramUniform1i(*iblProgram, loc, 5);
		glBindTextureUnit(5, scene.skyIrradiance);

		glBindVertexArray(emptyVertexArray);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_DEPTH_TEST);
	}

	if (ImGui::CollapsingHeader("IBL settings"))
	{
		ImGui::Checkbox("Indirect light", &indirectLight);
	}
}