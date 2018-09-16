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

	if (!directIBLProgram)
	{
		directIBLProgram = ShaderSystem::AddProgram("light/ibl.vert.glsl", "light/ibl.frag.glsl");
		glProgramUniform1i(*directIBLProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
		glProgramUniform1i(*directIBLProgram, PredefinedUniformLocation(u_g_buffer_normal), 1);
		glProgramUniform1i(*directIBLProgram, PredefinedUniformLocation(u_g_buffer_depth), 2);
	}

	glBindTextureUnit(0, gBuffer.albedoTexture);
	glBindTextureUnit(1, gBuffer.normalTexture);
	glBindTextureUnit(2, gBuffer.depthTexture);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightBuffer.framebuffer);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(*directIBLProgram);
	{
		glDisable(GL_DEPTH_TEST);

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