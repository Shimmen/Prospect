#include "LightPass.h"

#include <imgui.h>

#include "Scene.h"
#include "GuiSystem.h"
#include "TextureSystem.h"

using namespace glm;
#include "shader_locations.h"
#include "shader_types.h"

void
LightPass::Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, const ShadowMap& shadowMap, Scene& scene)
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

	if (!directionalLightUniformBuffer)
	{
		glCreateBuffers(1, &directionalLightUniformBuffer);
		glNamedBufferStorage(directionalLightUniformBuffer, sizeof(DirectionalLight), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glBindBufferBase(GL_UNIFORM_BUFFER, PredefinedUniformBlockBinding(DirectionalLightBlock), directionalLightUniformBuffer);
	}

	// TODO: We shouldn't need to do this every loop, right?
	glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
	glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_g_buffer_normal), 1);
	glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_g_buffer_depth), 2);

	// Bind the g-buffer
	glBindTextureUnit(0, gBuffer.albedoTexture);
	glBindTextureUnit(1, gBuffer.normalTexture);
	glBindTextureUnit(2, gBuffer.depthTexture);

	// Bind the shadow map
	glBindTextureUnit(10, shadowMap.texture);
	glProgramUniform1i(*directionalLightProgram, PredefinedUniformLocation(u_shadow_map), 10);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightBuffer.framebuffer);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(*directionalLightProgram);
	{
		assert(scene.directionalLights.size() == 1);
		auto& dirLight = scene.directionalLights[0];

		dirLight.viewDirecion = scene.mainCamera.GetViewMatrix() * dirLight.worldDirection;
		glNamedBufferSubData(directionalLightUniformBuffer, 0, sizeof(DirectionalLight), &dirLight);

		glDisable(GL_DEPTH_TEST);

		glBindVertexArray(emptyVertexArray);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_DEPTH_TEST);
	}

	if (ImGui::CollapsingHeader("Light buffer"))
	{
		GuiSystem::Texture(lightBuffer.lightTexture);
	}
}