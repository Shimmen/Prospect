#include "GeometryPass.h"

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "Maths.h"
#include "Logging.h"
#include "Material.h"
#include "GuiSystem.h"
#include "TransformSystem.h"

#include "shader_locations.h"

int
model_compare_function(const void *a, const void *b)
{
	auto modelA = (const Model *)a;
	auto modelB = (const Model *)b;

	if (modelA->material->program <  modelB->material->program) return -1;
	if (modelA->material->program == modelB->material->program) return  0;
	if (modelA->material->program >  modelB->material->program) return +1;

	return 0;
}

void
GeometryPass::Draw(const GBuffer& gBuffer, Scene& scene)
{
	std::array<glm::vec4, 6> frustumPlanes{};
	ExtractFrustumPlanes(scene.mainCamera.GetViewProjectionMatrix(), frustumPlanes);

	static std::vector<Model> geometryToRender{};
	geometryToRender.reserve(scene.models.size());
	geometryToRender.clear();

	for (const Model& model : scene.models)
	{
		if (!model.material->opaque) continue;

		auto transform = TransformSystem::Get(model.transformID);
		BoundingSphere worldSpaceBounds = model.bounds;
		worldSpaceBounds.center += transform.position;
		if (!InsideFrustum(frustumPlanes, worldSpaceBounds)) continue;

		geometryToRender.emplace_back(model);
	}

	const uint8_t magenta[] = { 255, 0, 255, 255 };
	glClearTexImage(gBuffer.albedoTexture, 0, GL_RGBA, GL_UNSIGNED_BYTE, magenta);

	const uint8_t black[] = { 0, 0, 0, 255 };
	glClearTexImage(gBuffer.normalTexture, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);

	const float farDepth = 1.0f;
	glClearTexImage(gBuffer.depthTexture, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &farDepth);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer.framebuffer);
	glViewport(0, 0, gBuffer.width, gBuffer.height);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	if (performDepthPrepass)
	{
		glDepthMask(true);
		glColorMask(false, false, false, false);

		if (!depthOnlyProgram)
		{
			depthOnlyProgram = ShaderSystem::AddProgram("material/depth_only");
		}

		glUseProgram(*depthOnlyProgram);
		GLint modelMatrixLoc = glGetUniformLocation(*depthOnlyProgram, "u_world_from_local"); // TODO: Use the predefined location!
		for (const Model& model : geometryToRender)
		{
			// TODO: Use linear uniform buffer for transforms instead
			Transform& transform = TransformSystem::Get(model.transformID);
			glUniformMatrix4fv(modelMatrixLoc, 1, false, glm::value_ptr(transform.matrix));

			if (model.material->cullBackfaces) glEnable(GL_CULL_FACE);
			else glDisable(GL_CULL_FACE);

			model.Draw();
		}

		glColorMask(true, true, true, true);
	}

	// Sort geometry so that we can optimize the number of shader program switches, i.e. calling glUseProgram
	qsort((void *)(geometryToRender.data()), geometryToRender.size(), sizeof(Model), model_compare_function);

	if (performDepthPrepass)
	{
		glDepthMask(false);
		glDepthFunc(GL_EQUAL);
	}

	int numDrawCalls = 0;
	int numTriangles = 0;

	GLuint lastProgram = UINT_MAX;
	for (const Model& model : geometryToRender)
	{
		GLuint program = model.material->program;

		if (program == 0)
		{
			continue;
		}

		if (program != lastProgram)
		{
			glUseProgram(program);
			lastProgram = program;
		}

		Transform& transform = TransformSystem::Get(model.transformID);
		model.material->BindUniforms(transform);

		if (model.material->cullBackfaces) glEnable(GL_CULL_FACE);
		else glDisable(GL_CULL_FACE);

		model.Draw();

		numDrawCalls += 1;
		numTriangles += TriangleCount(model);
	}

	glDepthMask(true);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);

	if (ImGui::CollapsingHeader("G-Buffer"))
	{
		ImGui::Checkbox("Perform depth-prepass", &performDepthPrepass);

		if (performDepthPrepass) ImGui::Text("Draw calls: %d (with depth-prepass)", 2 * numDrawCalls);
		else ImGui::Text("Draw calls: %d", numDrawCalls);
		ImGui::Text("Triangles:  %d", numTriangles);

		ImGui::Separator();

		ImGui::Text("Albedo:");
		GuiSystem::Texture(gBuffer.albedoTexture);
		ImGui::Text("Normal:");
		GuiSystem::Texture(gBuffer.normalTexture);
		ImGui::Text("Depth:");
		GuiSystem::Texture(gBuffer.depthTexture);
	}
}
