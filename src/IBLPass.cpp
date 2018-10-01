#include "IBLPass.h"

#include <random>

#include <imgui.h>

#include "Scene.h"
#include "GuiSystem.h"
#include "TextureSystem.h"

using namespace glm;
#include "shader_locations.h"
#include "shader_constants.h"
#include "shader_types.h"

void
IBLPass::Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, Scene& scene)
{
	if (!emptyVertexArray)
	{
		glCreateVertexArrays(1, &emptyVertexArray);
	}

	if (!iblProgram)
	{
		ShaderSystem::AddProgram("light/ibl", this);
	}

	if (!brdfIntegrationMap) { CreateBrdfIntegrationMap(); }
	if (!scene.skyIrradiance) { scene.skyIrradiance = TextureSystem::CreatePlaceholder(); }
	if (!scene.skyRadiance) { scene.skyRadiance = TextureSystem::CreatePlaceholder(); }

	if (ImGui::CollapsingHeader("Image Based Lighting (IBL)"))
	{
		if (ImGui::Button("Filter!"))
		{
			// TODO: Instead trigger from when both texture and model systems are 'idle', i.e. are done loading everything in
			scene.skyRadiance = FilterRadianceMap(scene.skyTexture);
		}

		ImGui::Text("Diffuse irradiance");
		GuiSystem::Texture(scene.skyIrradiance, 2.0f);

		ImGui::Text("Specular radiance");
		GuiSystem::Texture(scene.skyRadiance, 2.0f);

		static bool overrideRoughness = false;
		bool toggled = ImGui::Checkbox("Overide min roughness", &overrideRoughness);

		if (overrideRoughness)
		{
			static float roughness = 0.0f;
			ImGui::SliderFloat("Manual min roughness", &roughness, 0.0f, 1.0f);

			float minLod = roughness * float(IBL_RADIANCE_MIPMAP_LAYERS - 1);
			glTextureParameterf(scene.skyRadiance, GL_TEXTURE_MIN_LOD, minLod);
		}
		else if (toggled)
		{
			glTextureParameterf(scene.skyRadiance, GL_TEXTURE_MIN_LOD, -1'000.0f);
		}

		ImGui::Text("BRDF integration map");
		GuiSystem::Texture(brdfIntegrationMap, 1.0f);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightBuffer.framebuffer);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(iblProgram);
	glDisable(GL_DEPTH_TEST);

	glBindTextureUnit(0, gBuffer.albedoTexture);
	glBindTextureUnit(1, gBuffer.materialTexture);
	glBindTextureUnit(2, gBuffer.normalTexture);
	glBindTextureUnit(3, gBuffer.depthTexture);

	glBindTextureUnit(5, scene.skyIrradiance);
	glBindTextureUnit(6, scene.skyRadiance);

	glBindVertexArray(emptyVertexArray);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glEnable(GL_DEPTH_TEST);
}

void
IBLPass::ProgramLoaded(GLuint program)
{
	iblProgram = program;

	glProgramUniform1i(iblProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
	glProgramUniform1i(iblProgram, PredefinedUniformLocation(u_g_buffer_material), 1);
	glProgramUniform1i(iblProgram, PredefinedUniformLocation(u_g_buffer_normal), 2);
	glProgramUniform1i(iblProgram, PredefinedUniformLocation(u_g_buffer_depth), 3);

	GLint locIrradiance = glGetUniformLocation(iblProgram, "u_irradiance");
	glProgramUniform1i(iblProgram, locIrradiance, 5);

	GLint locRadiance = glGetUniformLocation(iblProgram, "u_radiance");
	glProgramUniform1i(iblProgram, locRadiance, 6);
}

void
IBLPass::CreateBrdfIntegrationMap()
{
	const int size = 512;

	GLuint map;
	glCreateTextures(GL_TEXTURE_2D, 1, &map);
	glTextureStorage2D(map, 1, GL_RG16F, size, size);

	glTextureParameteri(map, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(map, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(map, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(map, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLuint unit = 0;
	glBindImageTexture(unit, map, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG16F);

	GLuint program = *ShaderSystem::AddComputeProgram("compute/brdf_map.comp.glsl");

	glUseProgram(program);
	glDispatchCompute(size, size, 1);

	// NOTE: This might not be required here, since we don't need the result immediatly
	//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	brdfIntegrationMap = map;
}

GLuint
IBLPass::FilterRadianceMap(GLuint radianceMap)
{
	const GLenum format = GL_RGBA16F;

	// First, make sure the radiance map is square and generate mipmaps for it!
	// (for now we just assume that radianceMap is not square)

	GLuint squareMap;
	glCreateTextures(GL_TEXTURE_2D, 1, &squareMap);

	int numMipmaps = static_cast<int>(std::log2(IBL_RADIANCE_BASE_SIZE));
	glTextureStorage2D(squareMap, numMipmaps, format, IBL_RADIANCE_BASE_SIZE, IBL_RADIANCE_BASE_SIZE);

	glTextureParameteri(squareMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(squareMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(squareMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(squareMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	static GLuint *resizeProgram = ShaderSystem::AddComputeProgram("compute/resize.comp.glsl");

	glProgramUniform1i(*resizeProgram, glGetUniformLocation(*resizeProgram, "u_source"), 0);
	glBindTextureUnit(0, radianceMap);

	glBindImageTexture(0, squareMap, 0, GL_FALSE, 0, GL_WRITE_ONLY, format);

	glUseProgram(*resizeProgram);
	glDispatchCompute(IBL_RADIANCE_BASE_SIZE / 32, IBL_RADIANCE_BASE_SIZE / 32, 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glGenerateTextureMipmap(squareMap);

	//

	const int numLevels = IBL_RADIANCE_MIPMAP_LAYERS;

	GLuint filteredMap;
	glCreateTextures(GL_TEXTURE_2D, 1, &filteredMap);
	glTextureStorage2D(filteredMap, numLevels, format, IBL_RADIANCE_BASE_SIZE, IBL_RADIANCE_BASE_SIZE);

	glTextureParameteri(filteredMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(filteredMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(filteredMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(filteredMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	static GLuint *program = ShaderSystem::AddComputeProgram("compute/filter_radiance.comp.glsl");
	glUseProgram(*program);

	glProgramUniform1i(*program, glGetUniformLocation(*program, "u_radiance"), 0);
	glBindTextureUnit(0, squareMap);

	GLint roughnessLocation = glGetUniformLocation(*program, "u_roughness");

	for (int level = 0; level < numLevels; ++level)
	{
		float roughness = float(level) / float(numLevels - 1);

		glProgramUniform1f(*program, roughnessLocation, roughness);
		glBindImageTexture(0, filteredMap, level, GL_FALSE, 0, GL_WRITE_ONLY, format);

		int textureSize = IBL_RADIANCE_BASE_SIZE / int(pow(2, level));
		int localSize = IBL_RADIANCE_OPT_LOCAL_SIZE;
		glDispatchCompute(textureSize / localSize, textureSize / localSize, 1);
	}

	return filteredMap;
}
