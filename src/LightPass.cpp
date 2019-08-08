#include "LightPass.h"

#include <imgui.h>

#include "Scene.h"
#include "GuiSystem.h"
#include "TextureSystem.h"
#include "FullscreenQuad.h"

using namespace glm;
#include "shader_locations.h"
#include "shader_types.h"

void
LightPass::Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, const ShadowMap& shadowMap, Scene& scene)
{
	if (!directionalLightProgram)
	{
		ShaderSystem::AddProgram("light/directional", this);
	}

	if (!directionalLightUniformBuffer)
	{
		glCreateBuffers(1, &directionalLightUniformBuffer);
		glNamedBufferStorage(directionalLightUniformBuffer, sizeof(DirectionalLight), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glBindBufferBase(GL_UNIFORM_BUFFER, PredefinedUniformBlockBinding(DirectionalLightBlock), directionalLightUniformBuffer);
	}

	// Bind the g-buffer
	glBindTextureUnit(0, gBuffer.albedoTexture);
	glBindTextureUnit(1, gBuffer.materialTexture);
	glBindTextureUnit(2, gBuffer.normVelTexture);
	glBindTextureUnit(3, gBuffer.depthTexture);

	// Bind the shadow map
	glBindTextureUnit(10, shadowMap.texture);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightBuffer.framebuffer);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(directionalLightProgram);
	{
		assert(scene.directionalLights.size() == 1);
		auto& dirLight = scene.directionalLights[0];

		dirLight.viewDirecion = scene.mainCamera->GetViewMatrix() * dirLight.worldDirection;
		glNamedBufferSubData(directionalLightUniformBuffer, 0, sizeof(DirectionalLight), &dirLight);

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquation(GL_FUNC_ADD);

		glDisable(GL_DEPTH_TEST);

		FullscreenQuad::Draw();

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}

	if (ImGui::CollapsingHeader("Light pass"))
	{
		ImGui::SliderFloat("Sun intensity", &scene.directionalLights[0].color.a, 0.0f, 1.0f);
		GuiSystem::Texture(lightBuffer.lightTexture);
	}
}

void LightPass::ProgramLoaded(GLuint program)
{
	directionalLightProgram = program;

	glProgramUniform1i(directionalLightProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
	glProgramUniform1i(directionalLightProgram, PredefinedUniformLocation(u_g_buffer_material), 1);
	glProgramUniform1i(directionalLightProgram, PredefinedUniformLocation(u_g_buffer_norm_vel), 2);
	glProgramUniform1i(directionalLightProgram, PredefinedUniformLocation(u_g_buffer_depth), 3);

	glProgramUniform1i(directionalLightProgram, PredefinedUniformLocation(u_shadow_map), 10);
}
