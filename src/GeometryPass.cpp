#include "GeometryPass.h"

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

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
	// All geometry is currently opaque! Maybe later add some material flag to indicate opaqueness?
	auto opaqueGeometry = std::vector<Model>(scene.models);

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
		for (const Model& model : opaqueGeometry)
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
	qsort((void *)(opaqueGeometry.data()), opaqueGeometry.size(), sizeof(Model), model_compare_function);

	if (performDepthPrepass)
	{
		glDepthMask(false);
		glDepthFunc(GL_EQUAL);
	}

	GLuint lastProgram = UINT_MAX;
	for (const Model& model : opaqueGeometry)
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
	}

	glDepthMask(true);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);

	if (ImGui::CollapsingHeader("G-Buffer"))
	{
		ImGui::Text("Albedo:");
		GuiSystem::Texture(gBuffer.albedoTexture);
		ImGui::Text("Normal:");
		GuiSystem::Texture(gBuffer.normalTexture);
		ImGui::Text("Depth:");
		GuiSystem::Texture(gBuffer.depthTexture);
	}
}
