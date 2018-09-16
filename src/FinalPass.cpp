#include "FinalPass.h"

#include <imgui.h>

#include "GuiSystem.h"
#include "ShaderSystem.h"

#include "shader_locations.h"

void
FinalPass::Draw(const LightBuffer& lightBuffer)
{
	if (!emptyVertexArray)
	{
		glCreateVertexArrays(1, &emptyVertexArray);
	}

	if (!finalProgram)
	{
		finalProgram = ShaderSystem::AddProgram("quad.vert.glsl", "post/final.frag.glsl");
	}

	{
		float lastExposure = 0.0f;
		static float exposure = 1.0f;

		if (ImGui::CollapsingHeader("Postprocess"))
		{
			ImGui::SliderFloat("Exposure", &exposure, 0.0f, 32.0f, "%.1f");
		}

		if (exposure != lastExposure)
		{
			GLint loc = glGetUniformLocation(*finalProgram, "u_exposure");
			glProgramUniform1f(*finalProgram, loc, exposure);
			lastExposure = exposure;
		}
	}
	
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(*finalProgram);
	{
		glBindTextureUnit(10, lightBuffer.lightTexture);
		glProgramUniform1i(*finalProgram, PredefinedUniformLocation(u_texture), 10);

		glBindVertexArray(emptyVertexArray);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	glEnable(GL_DEPTH_TEST);
}
