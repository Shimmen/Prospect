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
		dirLightSegment.minX = 4096;
		dirLightSegment.minY = 4096;
		dirLightSegment.maxX = 8192;
		dirLightSegment.maxY = 8192;
		dirLightSegment.zNear = -100.0f;
		dirLightSegment.zFar = +100.0f;

		glm::vec3 L = glm::normalize(glm::vec3(0.2, 1.0, 0.2));
		auto dirLightView = glm::lookAtLH({ 0, 0, 0 }, -L, { 0, 1, 0 });

		float size = 50.0f;
		auto dirLightProjection = glm::orthoLH(-size, size, -size, size, dirLightSegment.zNear, dirLightSegment.zFar);

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
