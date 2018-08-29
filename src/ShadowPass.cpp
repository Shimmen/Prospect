#include "ShadowPass.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include "TransformSystem.h"
#include "ShaderSystem.h"
#include "Logging.h"

using namespace glm;
#include "shader_locations.h"
#include "shader_types.h"

struct ShadowMapSegmentUniformBufferData
{
	// TODO: Get max number of segments from somewhere
	// Or preferably, share this object somehow!
	// Also, see all hard coded 32s below...
	ShadowMapSegment shadowMapSegments[32];
	int shadowMapSegmentCount;
};

void
FillShadowMapSegmentBuffer(GLuint buffer, const std::vector<ShadowMapSegment>& shadowMapSegments)
{
	static ShadowMapSegmentUniformBufferData data{};

	size_t numSegments = shadowMapSegments.size();
	assert(numSegments <= 32);

	size_t numSegmentBytes = numSegments * sizeof(ShadowMapSegment);

	// fill data buffer
	std::memcpy(data.shadowMapSegments, shadowMapSegments.data(), numSegmentBytes);
	data.shadowMapSegmentCount = static_cast<int>(numSegments);

	// send to GPU
	glNamedBufferSubData(buffer, 0, sizeof(ShadowMapSegmentUniformBufferData), &data);
}

void
ShadowPass::Draw(const ShadowMap& shadowMap, const std::vector<Model>& blockingGeomety, DirectionalLight& dirLight)
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
		// TODO: Get size or max number of segments from somewhere
		size_t bufferSize = 32 * sizeof(ShadowMapSegment) + 1 * sizeof(int);

		glCreateBuffers(1, &shadowMapSegmentUniformBuffer);
		glNamedBufferStorage(shadowMapSegmentUniformBuffer, bufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
		glBindBufferBase(GL_UNIFORM_BUFFER, PredefinedUniformBlockBinding(ShadowMapSegmentBlock), shadowMapSegmentUniformBuffer);
	}

	if (!directionalLightUniformBuffer)
	{
		glCreateBuffers(1, &directionalLightUniformBuffer);
		glNamedBufferStorage(directionalLightUniformBuffer, sizeof(DirectionalLight), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glBindBufferBase(GL_UNIFORM_BUFFER, PredefinedUniformBlockBinding(DirectionalLightBlock), directionalLightUniformBuffer);
	}

	//
	// Reserve segments/parts of the shadow map for the different lights
	//

	std::vector<ShadowMapSegment> shadowMapSegments;
	shadowMapSegments.reserve(lightCount);

	// TODO/REMOVE: for the directional light
	int index = (int)shadowMapSegments.size();
	shadowMapSegments.push_back(dirLightSegment);
	dirLight.shadowMapSegmentIndex = ivec4(index, 0, 0, 0);

	FillShadowMapSegmentBuffer(shadowMapSegmentUniformBuffer, shadowMapSegments);
	glNamedBufferSubData(directionalLightUniformBuffer, 0, sizeof(DirectionalLight), &dirLight);

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

		for (const Model& model : blockingGeomety)
		{
			Transform& transform = TransformSystem::Get(model.transformID);
			glUniformMatrix4fv(PredefinedUniformLocation(u_world_from_local), 1, false, glm::value_ptr(transform.matrix));
			model.Draw();
		}
	}

	glCullFace(GL_BACK);
	glUseProgram(0);

}
