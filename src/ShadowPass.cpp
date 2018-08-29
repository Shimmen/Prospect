#include "ShadowPass.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include "TransformSystem.h"
#include "ShaderSystem.h"
#include "GuiSystem.h"
#include "Logging.h"

using namespace glm;
#include "shader_locations.h"
#include "shader_constants.h"
#include "shader_types.h"

void
ShadowPass::Draw(const ShadowMap& shadowMap, Scene& scene)
{
	// TODO: Pass in instead of setting up here!
	
	int lightCount = 1;

	ShadowMapSegment dirLightSegment;
	{
		int minX = 4096;
		int minY = 4096;
		int maxX = 8192;
		int maxY = 8192;

		dirLightSegment.minX = minX;
		dirLightSegment.minY = minY;
		dirLightSegment.maxX = maxX;
		dirLightSegment.maxY = maxY;

		vec2 textureSize = { shadowMap.size, shadowMap.size };
		vec2 minUV = vec2(minX, minY) / textureSize;
		vec2 maxUV = vec2(maxX, maxY) / textureSize;
		vec2 sizeUV = maxUV - minUV;

		mat4 toUnilateral = glm::translate(mat4(1.0f), vec3(0.5f)) * glm::scale(mat4(1.0f), vec3(0.5f));
		mat4 uvScale = glm::scale(mat4(1.0f), vec3(sizeUV, 0.0f));
		mat4 uvTranslation = glm::translate(mat4(1.0f), vec3(minUV, 0.0f));
		dirLightSegment.uvTransform = uvTranslation * uvScale * toUnilateral;

		auto dirLightView = glm::lookAtLH({ 0, 0, 0 }, vec3(scene.directionalLights[0].worldDirection), { 0, 1, 0 });

		float size = 50.0f;
		float zNear = -100.0f;
		float zFar = +100.0f;
		auto dirLightProjection = glm::orthoLH(-size, size, -size, size, zNear, zFar);

		dirLightSegment.lightViewProjection = dirLightProjection * dirLightView;
	}

	//

	if (!shadowProgram)
	{
		shadowProgram = ShaderSystem::AddProgram("material/shadow");
	}

	if (!shadowMapSegmentUniformBuffer)
	{
		size_t bufferSize = SHADOW_MAP_SEGMENT_MAX_COUNT * sizeof(ShadowMapSegment);

		glCreateBuffers(1, &shadowMapSegmentUniformBuffer);
		glNamedBufferStorage(shadowMapSegmentUniformBuffer, bufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
		glBindBufferBase(GL_UNIFORM_BUFFER, PredefinedUniformBlockBinding(ShadowMapSegmentBlock), shadowMapSegmentUniformBuffer);
	}

	//
	// Reserve segments/parts of the shadow map for the different lights
	//

	std::vector<ShadowMapSegment> shadowMapSegments;
	shadowMapSegments.reserve(lightCount);

	// TODO/REMOVE: for the directional light
	int index = (int)shadowMapSegments.size();
	shadowMapSegments.push_back(dirLightSegment);
	scene.directionalLights[0].shadowMapSegmentIndex = ivec4(index, 0, 0, 0);

	size_t numSegments = shadowMapSegments.size();
	assert(numSegments <= SHADOW_MAP_SEGMENT_MAX_COUNT);
	glNamedBufferSubData(shadowMapSegmentUniformBuffer, 0, numSegments * sizeof(ShadowMapSegment), shadowMapSegments.data());

	//
	// Render the shadow maps into the set
	//

	const float farDepth = 1.0f;
	glClearTexImage(shadowMap.texture, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &farDepth);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowMap.framebuffer);
	glUseProgram(*shadowProgram);

	// cull front faces to avoid shadow acne a bit
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	for (const ShadowMapSegment& segment : shadowMapSegments)
	{
		int width = segment.maxX - segment.minX;
		int height = segment.maxY - segment.minY;
		glViewport(segment.minX, segment.minY, width, height);

		glUniformMatrix4fv(PredefinedUniformLocation(u_projection_from_world), 1, false, glm::value_ptr(segment.lightViewProjection));

		for (const Model& model : scene.models)
		{
			Transform& transform = TransformSystem::Get(model.transformID);
			glUniformMatrix4fv(PredefinedUniformLocation(u_world_from_local), 1, false, glm::value_ptr(transform.matrix));
			model.Draw();
		}
	}

	glCullFace(GL_BACK);
	glUseProgram(0);

	if (ImGui::CollapsingHeader("Shadows"))
	{
		ImGui::Text("Shadow map size: %dx%d", shadowMap.size, shadowMap.size);
		GuiSystem::Texture(shadowMap.texture, 1.0f);
	}
}

ShadowMapSegment
ShadowPass::CreateShadowMapSegmentForDirectionalLight(const ShadowMap& shadowMap, const DirectionalLight& dirLight)
{
	ShadowMapSegment segment;
	
	int minX = 4096;
	int minY = 4096;
	int maxX = 8192;
	int maxY = 8192;

	segment.minX = minX;
	segment.minY = minY;
	segment.maxX = maxX;
	segment.maxY = maxY;

	vec2 textureSize = { shadowMap.size, shadowMap.size };
	vec2 minUV = vec2(minX, minY) / textureSize;
	vec2 maxUV = vec2(maxX, maxY) / textureSize;
	vec2 sizeUV = maxUV - minUV;

	static const mat4 id = mat4(1.0f);

	mat4 toUnilateral = glm::translate(id, vec3(0.5f)) * glm::scale(id, vec3(0.5f));
	mat4 uvScale = glm::scale(id, vec3(sizeUV, 0.0f));
	mat4 uvTranslation = glm::translate(id, vec3(minUV, 0.0f));
	segment.uvTransform = uvTranslation * uvScale * toUnilateral;

	auto dirLightView = glm::lookAtLH({ 0, 0, 0 }, vec3(dirLight.worldDirection), { 0, 1, 0 });

	float size = 50.0f;
	float zNear = -100.0f;
	float zFar = +100.0f;
	auto dirLightProjection = glm::orthoLH(-size, size, -size, size, zNear, zFar);

	segment.lightViewProjection = dirLightProjection * dirLightView;

	return segment;
}
