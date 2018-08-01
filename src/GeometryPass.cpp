#include "GeometryPass.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>

#include "Logging.h"
#include "Material.h"
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
GeometryPass::Draw(const GBuffer& gBuffer, const std::vector<Model>& opaqueGeometry, FpsCamera& camera)
{
	// Prepare framebuffer
	{
		if (!framebuffer)
		{
			glCreateFramebuffers(1, &framebuffer);

			GLenum drawBuffers[] = {
				PredefinedOutputLocation(o_g_buffer_albedo),
				PredefinedOutputLocation(o_g_buffer_normal)
			};
			int numDrawBuffers = sizeof(drawBuffers) / sizeof(GLenum);
			glNamedFramebufferDrawBuffers(framebuffer, numDrawBuffers, drawBuffers);
		}

		if (gBuffer.albedoTexture != lastBoundAlbedo)
		{
			GLenum albedoAttachment = PredefinedOutputLocation(o_g_buffer_albedo);
			glNamedFramebufferTexture(framebuffer, albedoAttachment, gBuffer.albedoTexture, 0);
			lastBoundAlbedo = gBuffer.albedoTexture;
		}

		if (gBuffer.normalTexture != lastBoundNormal)
		{
			int normalAttachment = PredefinedOutputLocation(o_g_buffer_normal);
			glNamedFramebufferTexture(framebuffer, normalAttachment, gBuffer.normalTexture, 0);
			lastBoundNormal = gBuffer.normalTexture;
		}

		if (gBuffer.depthTexture != lastBoundDepth)
		{
			glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, gBuffer.depthTexture, 0);
			lastBoundDepth = gBuffer.depthTexture;
		}

		GLenum status = glCheckNamedFramebufferStatus(framebuffer, GL_DRAW_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			LogError("G-buffer is not complete for the geometry pass!");
		}

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	}

	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
		GLint modelMatrixLoc = glGetUniformLocation(*depthOnlyProgram, "u_world_from_local");
		for (const Model& model : opaqueGeometry)
		{
			// TODO: Use linear uniform buffer for transforms instead
			Transform& transform = TransformSystem::Get(model.transformID);
			glUniformMatrix4fv(modelMatrixLoc, 1, false, glm::value_ptr(transform.matrix));
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

		model.Draw();
	}

	glDepthMask(true);
	glDepthFunc(GL_LEQUAL);

}
